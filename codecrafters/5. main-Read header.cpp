#include <cstddef>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <map>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>
#define MAX_CONNECTIONS 10
#define BUFFER_SIZE 1024
struct HTTPResponse {
    std::string status;
    std::string content_type;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string to_string() {
        std::string response;
        response += status + "\r\n";
        response += "Content-Type: " + content_type + "\r\n";
        for (const auto& header : headers) {
            response += header.first + ": " + header.second + "\r\n";
        }
        response += "\r\n";
        response += body;
        return response;
    }
};
struct HTTPRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
};
HTTPRequest parse_request(std::string request) {
    HTTPRequest req;
    std::stringstream ss(request);
    std::string line;
    std::getline(ss, line);
    std::istringstream line_ss(line);
    line_ss >> req.method >> req.path >> req.version;
    while (std::getline(ss, line) && !line.empty()) {
        size_t pos = line.find(":");
        if (pos != std::string::npos) {
            std::string header_name = line.substr(0, pos);
            std::string header_value = line.substr(pos + 2);
            header_value.erase(header_value.end() - 1);
            req.headers[header_name] = header_value;
        }
    }
    std::stringstream body_ss;
    std::getline(ss, line);
    body_ss << line;
    while (std::getline(ss, line)) {
        body_ss << "\n" << line;
    }
    req.body = body_ss.str();
    return req;
}
void write_response(int client_fd, HTTPResponse response) {
    std::string response_str = response.to_string();
    ssize_t bytes_written = write(client_fd, response_str.c_str(), response_str.size());
    if (bytes_written < 0) {
        std::cerr << "Failed to write response to client\n";
        close(client_fd);
    }
}
void handle_client(int client_fd, int epoll_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer));
    if (bytes_read <= 0) {
        std::cerr << "Failed to read from client\n";
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        close(client_fd);
        return;
    }
    std::string message(buffer, bytes_read);
    std::cout << "Received message: " << message << std::endl;
    HTTPRequest request = parse_request(message);
    if (request.method == "GET") {
        if (request.path == "/") {
            HTTPResponse response = { "HTTP/1.1 200 OK", "text/plain", {}, "Hello, World!" };
            write_response(client_fd, response);
        } else if (request.path == "/user-agent") {
            std::string body = request.headers["User-Agent"];
            HTTPResponse response = { "HTTP/1.1 200 OK", "text/plain", { {"Content-Length", std::to_string(body.length())} }, body };
            write_response(client_fd, response);
        } else if (request.path.substr(0, 6) == "/echo/") {
            std::string subStr = request.path.substr(6);
            HTTPResponse response = { "HTTP/1.1 200 OK", "text/plain", { {"Content-Length", std::to_string(subStr.length())} }, subStr };
            write_response(client_fd, response);
        } else if (request.path.substr(0, 7) == "/files/") {
            std::string directory = ".";
            std::string filename = request.path.substr(7);
            std::ifstream file(directory + "/" + filename);
            if (!file) {
                HTTPResponse response = { "HTTP/1.1 404 Not Found", "text/plain", {}, "Not Found" };
                write_response(client_fd, response);
                return;
            }
            std::stringstream body;
            body << file.rdbuf();
            HTTPResponse response = { "HTTP/1.1 200 OK", "application/octet-stream", { {"Content-Length", std::to_string(body.str().length())} }, body.str() };
            write_response(client_fd, response);
        } else {
            HTTPResponse response = { "HTTP/1.1 404 Not Found", "text/plain", {}, "Not Found" };
            write_response(client_fd, response);
        }
    } else {
        HTTPResponse response = { "HTTP/1.1 405 Method Not Allowed", "text/plain", {}, "Method Not Allowed" };
        write_response(client_fd, response);
    }
}
int main(int argc, char **argv) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket\n";
        return 1;
    }
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt failed\n";
        return 1;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(4221);
    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
        std::cerr << "Failed to bind to port 4221\n";
        return 1;
    }
    int connection_backlog = 10;
    if (listen(server_fd, connection_backlog) != 0) {
        std::cerr << "listen failed\n";
        return 1;
    }
    // Create an epoll instance
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "Failed to create epoll instance\n";
        return 1;
    }
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        std::cerr << "Failed to add server socket to epoll instance\n";
        return 1;
    }
    struct epoll_event events[MAX_CONNECTIONS];
    while (true) {
        int num_fds = epoll_wait(epoll_fd, events, MAX_CONNECTIONS, -1);
        for (int i=0; i < num_fds; i++) {
            if (events[i].data.fd == server_fd) {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                // Accept the connection
                int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
                if (client_fd == -1) {
                    std::cerr << "Failed to accept connection\n";
                    continue;
                }
                std::cout << "Client connected\n";
                event.events = EPOLLIN;
                event.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                    std::cerr << "Failed to add client socket to epoll instance\n";
                    close(client_fd);
                    continue;
                }
            } else {
                handle_client(events[i].data.fd, epoll_fd);
            }
        }
    }
    close(server_fd);
    return 0;
}

/*
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

int connect_server() {
  // Flush after every std::cout / std::cerr
  cout << unitbuf;
  cerr << unitbuf;

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    cerr << "Failed to create server socket\n";
    return 1;
  }

  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
      0) {
    cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) !=
      0) {
    cerr << "Failed to bind to port 4221\n";
    close(server_fd);
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    cerr << "listen failed\n";
    close(server_fd);
    return 1;
  }

  return server_fd;
}

int connect_client(int &server_fd) {

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  cout << "Waiting for a client to connect...\n";

  int client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                         (socklen_t *)&client_addr_len);

  if (client_fd < 0) {
    cerr << "Failed to connect to client ...\n";
    close(server_fd);
    return 1;
  }
  cout << "Client connected\n";
  return client_fd;
}

int receive_request(int &client_fd, int &server_fd, string &msg) {

  ssize_t brecvd = recv(client_fd, (void *)&msg[0], msg.size(), 0);
  if (brecvd < 0) {
    cerr << "error receiving message from client" << endl;
    close(client_fd);
    close(server_fd);
    return 1;
  }
  msg.resize(brecvd);
  cerr << "Client message (length: " << brecvd << ")" << endl;
  clog << msg << endl;
  return 0;
}

string form_response(string &msg) {
  cout << msg << endl;
  stringstream ss;
  string ok = "HTTP/1.1 200 OK\r\n";
  string bad = "HTTP/1.1 404 Not Found\r\n";

  auto first_space = msg.find(' ');
  auto second_space = msg.find(' ', first_space + 1);
  string path = msg.substr(first_space + 1, second_space - first_space - 1);

  if ((second_space - first_space) == 2) {
    ss << ok << "\r\n\r\n";
    return ss.str();
  }

  if (path.substr(0, 6) == "/echo/") {
    ss << ok;
    ss << "Content-Type: text/plain\r\n";

    auto echo = path.substr(6, second_space);

    ss << "Content-Length: " << echo.size() << " \r\n\r\n";
    ss << echo;
    return ss.str();
  }

  if (path == "/user-agent") {
    string prefix = "User-Agent: ";

    size_t start = msg.find(prefix);

    if (start != string::npos) {

      start += prefix.size();

      size_t end = msg.find("\r\n", start);

      string user_agent = msg.substr(start, end - start);
      ss << ok;
      ss << "Content-Type: text/plain\r\n";
      ss << "Content-Length: " << user_agent.size() << "\r\n\r\n";
      ss << user_agent << endl;
    }
    cout << ss.str() << endl;
    return ss.str();
  }

  ss << bad;
  ss << "\r\n\r\n";
  return ss.str();
}

int send_response(int &client_fd, int &server_fd, string &res) {
  ssize_t bsent = send(client_fd, res.c_str(), res.size(), 0);
  if (bsent < 0) {
    cerr << "Error sending response to client";
    close(client_fd);
    close(server_fd);
    return 1;
  }
  return 0;
}

int main(int argc, char **argv) {

  int server_fd = connect_server();
  if (server_fd == 1)
    return 1;

  while (true) {
    int client_fd = connect_client(server_fd);
    if (client_fd == 1)
      return 1;

    string msg(1024, '\0');
    if (receive_request(client_fd, server_fd, msg) == 1)
      return 1;

    string res = form_response(msg);
    if (send_response(client_fd, server_fd, res) == 1)
      return 1;

    close(client_fd);
  }
  close(server_fd);

  return 0;
}
*/