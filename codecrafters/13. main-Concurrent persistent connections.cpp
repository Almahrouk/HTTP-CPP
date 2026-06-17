#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <netdb.h>
#include <set>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <zlib.h>

constexpr int kPort = 4221;
constexpr int kBacklog = 5;
constexpr size_t kRecvBuf = 4096;

struct Req {
  std::string method, path, version;
  std::map<std::string, std::string> headers;
  std::string body;
};

struct Res {
  std::string version = "HTTP/1.1";
  std::string code, reason;
  std::map<std::string, std::string> headers;
  std::string body;
};

static Req parse(const std::string &request) {
  Req r;
  std::istringstream stream(request);
  stream >> r.method >> r.path >> r.version;
  stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  std::string line;
  while (std::getline(stream, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (line.empty())
      break;

    auto colon = line.find(':');
    if (colon == std::string::npos)
      continue;

    std::string key = line.substr(0, colon);
    std::string value = line.substr(colon + 1);
    auto start = value.find_first_not_of(" \t");
    value = (start != std::string::npos) ? value.substr(start) : "";
    r.headers[key] = value;
  }

  std::ostringstream body;
  body << stream.rdbuf();
  r.body = body.str();
  return r;
}

static std::string stringify(const Res &res) {
  std::string out = res.version + " " + res.code + " " + res.reason + "\r\n";
  for (const auto &[k, v] : res.headers)
    out += k + ": " + v + "\r\n";
  out += "\r\n" + res.body;
  return out;
}

static std::string gzip_compress(const std::string &data) {
  z_stream zs{};
  if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8,
                   Z_DEFAULT_STRATEGY) != Z_OK) {
    throw std::runtime_error("deflateInit2 failed");
  }
  zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.data()));
  zs.avail_in = static_cast<uInt>(data.size());

  std::string out;
  char buf[16384];
  int ret;
  do {
    zs.next_out = reinterpret_cast<Bytef *>(buf);
    zs.avail_out = sizeof(buf);
    ret = deflate(&zs, Z_FINISH);
    out.append(buf, sizeof(buf) - zs.avail_out);
  } while (ret == Z_OK);
  deflateEnd(&zs);
  if (ret != Z_STREAM_END)
    throw std::runtime_error("deflate failed");
  return out;
}

static Res text_response(std::string body, std::string encoding = "none") {
  Res res;
  res.code = "200";
  res.reason = "OK";
  res.headers["Content-Type"] = "text/plain";
  if (encoding != "none") {
    if (encoding == "gzip")
      body = gzip_compress(body);
    res.headers["Content-Encoding"] = std::move(encoding);
  }
  res.headers["Content-Length"] = std::to_string(body.size());
  res.body = std::move(body);
  return res;
}

static Res not_found() {
  Res res;
  res.code = "404";
  res.reason = "Not Found";
  return res;
}

static Res handle_get_file(const std::string &directory,
                           const std::string &path) {
  std::ifstream file(directory + path.substr(7), std::ios::binary);
  if (!file)
    return not_found();

  std::ostringstream ss;
  ss << file.rdbuf();
  Res res;
  res.code = "200";
  res.reason = "OK";
  res.body = ss.str();
  res.headers["Content-Type"] = "application/octet-stream";
  res.headers["Content-Length"] = std::to_string(res.body.size());
  return res;
}

static Res handle_post_file(const std::string &directory, const Req &req) {
  if (req.path.rfind("/files/", 0) != 0)
    return not_found();
  std::ofstream file(directory + req.path.substr(7));
  file << req.body;
  Res res;
  res.code = "201";
  res.reason = "Created";
  return res;
}

static Res route(const Req &req, const std::string &directory) {
  if (req.method == "POST")
    return handle_post_file(directory, req);
  if (req.path == "/") {
    Res res;
    res.code = "200";
    res.reason = "OK";
    return res;
  }
  if (req.path.rfind("/echo/", 0) == 0) {
    static const std::set<std::string> supported = {"gzip"};
    std::string body = req.path.substr(6);
    auto it = req.headers.find("Accept-Encoding");
    if (it != req.headers.end()) {
      std::istringstream ss(it->second);
      std::string tok;
      while (std::getline(ss, tok, ',')) {
        auto a = tok.find_first_not_of(" \t");
        auto b = tok.find_last_not_of(" \t");
        if (a == std::string::npos)
          continue;
        std::string enc = tok.substr(a, b - a + 1);
        if (supported.count(enc))
          return text_response(body, enc);
      }
    }
    return text_response(body);
  }
  if (req.path == "/user-agent") {
    auto it = req.headers.find("User-Agent");
    return text_response(it != req.headers.end() ? it->second : "");
  }
  if (req.path.rfind("/files/", 0) == 0)
    return handle_get_file(directory, req.path);
  return not_found();
}

static bool read_request(int fd, std::string &buf, Req &req) {
  while (true) {
    auto pos = buf.find("\r\n\r\n");
    if (pos != std::string::npos) {
      Req tmp = parse(buf.substr(0, pos + 4));
      size_t body_start = pos + 4;
      size_t cl = 0;
      auto it = tmp.headers.find("Content-Length");
      if (it != tmp.headers.end()) {
        try {
          cl = std::stoul(it->second);
        } catch (...) {
          return false;
        }
      }
      while (buf.size() - body_start < cl) {
        char tmpbuf[kRecvBuf];
        ssize_t n = recv(fd, tmpbuf, sizeof(tmpbuf), 0);
        if (n <= 0)
          return false;
        buf.append(tmpbuf, n);
      }
      tmp.body = buf.substr(body_start, cl);
      buf.erase(0, body_start + cl);
      req = std::move(tmp);
      return true;
    }
    char tmpbuf[kRecvBuf];
    ssize_t n = recv(fd, tmpbuf, sizeof(tmpbuf), 0);
    if (n <= 0)
      return false;
    buf.append(tmpbuf, n);
  }
}

static void handle_client(int client_fd, std::string directory) {
  std::string buf;
  while (true) {
    Req req;
    if (!read_request(client_fd, buf, req))
      break;

    Res res = route(req, directory);
    auto it = req.headers.find("Connection");
    bool close_after = (it != req.headers.end() && it->second == "close");
    if (close_after)
      res.headers["Connection"] = "close";

    std::string out = stringify(res);
    if (send(client_fd, out.c_str(), out.size(), 0) < 0)
      break;
    if (close_after)
      break;
  }
  close(client_fd);
}

static std::string parse_directory(int argc, char **argv) {
  for (int i = 1; i + 1 < argc; i++) {
    if (std::string(argv[i]) == "--directory")
      return argv[i + 1];
  }
  return "";
}

int main(int argc, char **argv) {
  std::string directory = parse_directory(argc, argv);

  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  std::cout << "Logs from your program will appear here!\n";

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }

  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
      0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(kPort);

  if (bind(server_fd, (sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port " << kPort << "\n";
    return 1;
  }

  if (listen(server_fd, kBacklog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }

  std::cout << "Waiting for clients to connect...\n";

  sockaddr_in client_addr{};
  socklen_t client_addr_len = sizeof(client_addr);

  while (true) {
    int client_fd =
        accept(server_fd, (sockaddr *)&client_addr, &client_addr_len);
    if (client_fd < 0) {
      perror("accept");
      continue;
    }
    std::thread(handle_client, client_fd, directory).detach();
  }

  close(server_fd);
  return 0;
}