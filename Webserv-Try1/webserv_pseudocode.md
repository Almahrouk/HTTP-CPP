# Webserv — Full Project Pseudocode (C++98)

---

## Directory Structure

```
webserv/
├── Makefile
├── webserv.conf                  ← default config
├── src/
│   ├── main.cpp
│   ├── config/
│   │   ├── Tokenizer.cpp/.hpp    ← raw text → tokens
│   │   ├── Parser.cpp/.hpp       ← tokens → ServerConfig objects
│   │   └── Config.cpp/.hpp       ← final config data structures
│   ├── server/
│   │   ├── Server.cpp/.hpp       ← socket lifecycle, poll() loop
│   │   ├── Client.cpp/.hpp       ← per-connection state machine
│   │   └── VirtualHost.cpp/.hpp  ← server_name matching
│   ├── http/
│   │   ├── Request.cpp/.hpp      ← HTTP request parser
│   │   ├── Response.cpp/.hpp     ← HTTP response builder
│   │   ├── Router.cpp/.hpp       ← URI → location matching
│   │   └── Methods.cpp/.hpp      ← GET / POST / DELETE handlers
│   ├── cgi/
│   │   └── CGI.cpp/.hpp          ← fork/execve/pipe/timeout
│   └── utils/
│       ├── Utils.cpp/.hpp        ← string helpers, MIME, path ops
│       └── Logger.cpp/.hpp       ← stderr logging
└── www/                          ← default web root
```

---

## A — Data Structures

```
/* ── Config layer ─────────────────────────────────────────── */

struct LocationConfig {
    string              path;           // "/", "/uploads", "/cgi"
    vector<string>      methods;        // ["GET","POST","DELETE"]
    string              root;           // filesystem root override
    string              index;          // default file
    bool                autoindex;      // directory listing
    long                client_max_body_size;
    string              redirect;       // "301 http://..."
    map<string,string>  cgi_extensions; // ".py" -> "/usr/bin/python3"
    string              upload_store;   // upload destination dir
}

struct ServerConfig {
    vector<int>             ports;          // [8080, 4242]
    string                  host;           // "0.0.0.0"
    vector<string>          server_names;   // ["localhost","webserv"]
    string                  root;
    string                  index;
    long                    client_max_body_size;  // bytes
    map<int,string>         error_pages;    // {404: "error/404.html"}
    vector<LocationConfig>  locations;
}

/* ── Per-connection state ──────────────────────────────────── */

enum ClientState {
    READING_REQUEST,
    PROCESSING,
    WRITING_RESPONSE,
    CGI_WAITING,
    DONE
}

struct Client {
    int             fd;
    ClientState     state;
    ServerConfig*   server;         // matched virtual host
    string          read_buf;       // raw incoming bytes
    string          write_buf;      // bytes waiting to be sent
    Request*        request;
    Response*       response;
    CGI*            cgi;            // NULL if not a CGI request
    time_t          last_activity;  // for timeout
}

/* ── Poll descriptor wrapper ───────────────────────────────── */

struct PollEntry {
    int         fd;
    short       events;     // POLLIN | POLLOUT
    short       revents;
    bool        is_server;  // listening socket vs client/cgi pipe
}
```

---

## B — Tokenizer  `src/config/Tokenizer.cpp`

```
TOKEN TYPES: WORD | OPEN_BRACE | CLOSE_BRACE | SEMICOLON

function tokenize(filepath) -> vector<Token>:
    open file, error if missing or unreadable
    for each line:
        strip comment (everything after '#')
        skip empty lines
        for each character:
            if '{' -> emit OPEN_BRACE
            if '}' -> emit CLOSE_BRACE
            if ';' -> emit SEMICOLON, flush current word
            if whitespace -> flush current word if non-empty
            else -> append to current word buffer
    if unclosed brace -> error("unexpected end of file")
    return tokens
```

---

## C — Parser  `src/config/Parser.cpp`

```
function parse(tokens) -> vector<ServerConfig>:
    configs = []
    while tokens not exhausted:
        expect token "server" (WORD)
        expect OPEN_BRACE
        configs.push( parse_server_block(tokens) )
    return configs

function parse_server_block(tokens) -> ServerConfig:
    cfg = ServerConfig()
    while next token != CLOSE_BRACE:
        directive = consume WORD
        switch directive:
            "listen"               -> cfg.ports.push( parse_int(consume WORD) )
                                      expect SEMICOLON
            "host"                 -> cfg.host = consume WORD
                                      expect SEMICOLON
            "server_name"          -> cfg.server_names = consume_until_semicolon()
            "root"                 -> cfg.root = consume WORD; expect SEMICOLON
            "index"                -> cfg.index = consume WORD; expect SEMICOLON
            "client_max_body_size" -> cfg.client_max_body_size = parse_size(consume WORD)
                                      expect SEMICOLON          // "10M" → 10*1024*1024
            "error_page"           -> code = parse_int(consume WORD)
                                      path = consume WORD
                                      cfg.error_pages[code] = path
                                      expect SEMICOLON
            "location"             -> path = consume WORD
                                      expect OPEN_BRACE
                                      cfg.locations.push( parse_location_block(path, tokens) )
            default                -> error("unknown directive: " + directive)
    expect CLOSE_BRACE
    validate_server_config(cfg)      // must have at least one port
    return cfg

function parse_location_block(path, tokens) -> LocationConfig:
    loc = LocationConfig()
    loc.path = path
    while next token != CLOSE_BRACE:
        directive = consume WORD
        switch directive:
            "methods"          -> loc.methods = consume_until_semicolon()
            "root"             -> loc.root = consume WORD; expect SEMICOLON
            "index"            -> loc.index = consume WORD; expect SEMICOLON
            "autoindex"        -> loc.autoindex = (consume WORD == "on")
                                  expect SEMICOLON
            "return"           -> loc.redirect = consume WORD + " " + consume WORD
                                  expect SEMICOLON
            "cgi_extension"    -> ext = consume WORD; bin = consume WORD
                                  loc.cgi_extensions[ext] = bin
                                  expect SEMICOLON
            "upload_store"     -> loc.upload_store = consume WORD
                                  expect SEMICOLON
            "client_max_body_size" -> loc.client_max_body_size = parse_size(consume WORD)
                                      expect SEMICOLON
            default            -> error("unknown location directive: " + directive)
    expect CLOSE_BRACE
    return loc
```

---

## D — Server Bootstrap  `src/server/Server.cpp`

```
class Server:
    vector<ServerConfig>    configs
    vector<PollEntry>       poll_fds      // master poll list
    map<int,ServerConfig*>  listen_map    // fd → config
    map<int,Client*>        client_map    // fd → Client

    function init(configs):
        for each ServerConfig cfg:
            for each port in cfg.ports:
                fd = socket(AF_INET, SOCK_STREAM, 0)
                setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, 1)
                fcntl(fd, F_SETFL, O_NONBLOCK)
                bind(fd, host:port)
                listen(fd, BACKLOG=128)
                add fd to poll_fds as POLLIN | is_server=true
                listen_map[fd] = &cfg
        log("Listening on " + all ports)

    function run():
        while true:
            ready = poll(poll_fds, timeout=5000ms)
            if ready < 0 and errno != EINTR -> error
            check_cgi_timeouts()
            check_client_timeouts()
            for each entry in poll_fds (iterate copy — list may grow):
                if entry.revents & POLLERR | POLLHUP -> close_client(fd)
                else if entry.is_server and entry.revents & POLLIN:
                    accept_new_connection(entry.fd)
                else if client_map has entry.fd:
                    client = client_map[entry.fd]
                    if entry.revents & POLLIN  -> read_from_client(client)
                    if entry.revents & POLLOUT -> write_to_client(client)
                else if cgi pipe fd:
                    read_cgi_output(entry.fd)

    function accept_new_connection(server_fd):
        client_fd = accept(server_fd, ...)
        fcntl(client_fd, F_SETFL, O_NONBLOCK)
        client = new Client(client_fd, listen_map[server_fd])
        client_map[client_fd] = client
        add client_fd to poll_fds as POLLIN

    function close_client(fd):
        delete client_map[fd]
        remove fd from poll_fds
        close(fd)

    function check_client_timeouts():
        now = time(NULL)
        for each client in client_map:
            if now - client.last_activity > 60 seconds:
                send_error_response(client, 408)
                close_client(client.fd)
```

---

## E — Request Parser  `src/http/Request.cpp`

```
class Request:
    string  method          // "GET" "POST" "DELETE" "HEAD"
    string  uri             // "/path?query"
    string  path            // "/path"
    string  query_string    // "query"
    string  version         // "HTTP/1.1"
    map<string,string> headers
    string  body
    bool    complete        // full request received?
    bool    chunked

    function feed(data):
        // append to internal buffer, then try to parse
        buffer += data
        if not headers_done:
            pos = buffer.find("\r\n\r\n")
            if pos == npos: return  // need more data
            parse_request_line_and_headers(buffer[0..pos])
            headers_done = true
            buffer = buffer[pos+4..]  // remainder is body start
        read_body()

    function parse_request_line_and_headers(head_section):
        lines = split(head_section, "\r\n")
        // Request line
        parts = split(lines[0], " ")
        if parts.size != 3 -> throw 400
        method  = parts[0]
        uri     = parts[1]
        version = parts[2]
        validate_method()   // must be GET/POST/DELETE/HEAD
        validate_version()  // must be HTTP/1.0 or HTTP/1.1
        parse_uri()         // split path and query_string
        // Headers
        for lines[1..]:
            colon = line.find(':')
            if colon == npos -> throw 400
            key = lowercase(trim(line[0..colon]))
            val = trim(line[colon+1..])
            headers[key] = val
        // Chunked?
        if headers["transfer-encoding"] == "chunked": chunked = true

    function read_body():
        if chunked:
            decode_chunked()
        else:
            length = parse_int(headers["content-length"] or "0")
            if buffer.size >= length:
                body = buffer[0..length]
                complete = true

    function decode_chunked():
        // parse hex size\r\n, data\r\n, repeat until 0\r\n\r\n
        while buffer not empty:
            crlf = buffer.find("\r\n")
            if crlf == npos: return
            chunk_size = hex_to_int(buffer[0..crlf])
            if chunk_size == 0: complete = true; return
            if buffer.size < crlf+2+chunk_size+2: return  // wait
            body += buffer[crlf+2 .. crlf+2+chunk_size]
            buffer = buffer[crlf+2+chunk_size+2 ..]
```

---

## F — Router  `src/http/Router.cpp`

```
function match_server(request, listen_fd, all_configs) -> ServerConfig*:
    host_header = request.headers["host"]  // strip port
    candidates  = all configs bound to listen_fd's port
    // Prefer exact server_name match
    for each cfg in candidates:
        if host_header in cfg.server_names: return cfg
    // Fall back to first config on that port
    return candidates[0]

function match_location(request_path, server) -> LocationConfig*:
    best_match = NULL
    best_len   = -1
    for each loc in server.locations:
        if request_path starts_with loc.path:
            if loc.path.length > best_len:
                best_match = &loc
                best_len   = loc.path.length
    return best_match   // NULL means use server defaults

function resolve_file_path(request_path, server, location) -> string:
    root = location.root if set else server.root
    // strip location prefix, prepend root
    relative = request_path.substr(location.path.length)
    return root + "/" + relative
```

---

## G — Method Handlers  `src/http/Methods.cpp`

```
/* ── GET ──────────────────────────────────────────────────── */
function handle_GET(request, server, location) -> Response:
    // 1. Redirect?
    if location.redirect not empty:
        return Response(301, location.redirect)

    // 2. CGI?
    ext = file_extension(request.path)
    if ext in location.cgi_extensions:
        return run_cgi(request, server, location)

    // 3. Resolve path
    fspath = resolve_file_path(request.path, server, location)

    // 4. Is directory?
    if is_directory(fspath):
        index_file = fspath + "/" + (location.index or server.index)
        if file_exists(index_file):
            fspath = index_file
        else if location.autoindex:
            return generate_autoindex(fspath, request.path)
        else:
            return error_response(403, server)

    // 5. File not found
    if not file_exists(fspath):
        return error_response(404, server)

    // 6. Permissions
    if not readable(fspath):
        return error_response(403, server)

    // 7. Serve file
    body = read_file(fspath)
    res  = Response(200)
    res.set_header("Content-Type",   mime_type(fspath))
    res.set_header("Content-Length", body.size)
    res.body = body
    return res

/* ── HEAD ─────────────────────────────────────────────────── */
function handle_HEAD(request, server, location) -> Response:
    // Identical to GET but body is cleared before sending
    res = handle_GET(request, server, location)
    res.body = ""
    // Content-Length header must stay (keep the value from GET)
    return res

/* ── POST ─────────────────────────────────────────────────── */
function handle_POST(request, server, location) -> Response:
    // 1. Body size check
    max = location.client_max_body_size or server.client_max_body_size
    if request.body.size > max:
        return error_response(413, server)

    // 2. CGI?
    ext = file_extension(request.path)
    if ext in location.cgi_extensions:
        return run_cgi(request, server, location)

    // 3. File upload to upload_store
    if location.upload_store not empty:
        filename = extract_filename(request)  // from Content-Disposition
        if filename empty: filename = generate_unique_name()
        dest = location.upload_store + "/" + filename
        write_file(dest, request.body)
        return Response(201, "Created")

    return error_response(405, server)

/* ── DELETE ───────────────────────────────────────────────── */
function handle_DELETE(request, server, location) -> Response:
    fspath = resolve_file_path(request.path, server, location)
    if not file_exists(fspath):
        return error_response(404, server)
    if not is_regular_file(fspath):
        return error_response(403, server)
    if unlink(fspath) != 0:
        return error_response(500, server)
    return Response(204)  // No Content, no body
```

---

## H — Response Builder  `src/http/Response.cpp`

```
class Response:
    int                     status_code
    string                  reason
    map<string,string>      headers
    string                  body
    bool                    headers_sent

    static map<int,string> STATUS = {
        200:"OK", 201:"Created", 204:"No Content",
        301:"Moved Permanently", 302:"Found",
        400:"Bad Request", 403:"Forbidden", 404:"Not Found",
        405:"Method Not Allowed", 408:"Request Timeout",
        413:"Payload Too Large", 500:"Internal Server Error",
        502:"Bad Gateway"
    }

    function serialize() -> string:
        out  = "HTTP/1.1 " + status_code + " " + STATUS[status_code] + "\r\n"
        headers["Content-Length"] = body.size
        if not headers.has("Content-Type"):
            headers["Content-Type"] = "text/html"
        headers["Server"] = "webserv/1.0"
        for each [k,v] in headers:
            out += k + ": " + v + "\r\n"
        out += "\r\n"
        out += body
        return out

function error_response(code, server) -> Response:
    // Use custom error page if configured
    if server.error_pages.has(code):
        path = server.root + "/" + server.error_pages[code]
        if file_exists(path):
            body = read_file(path)
            res  = Response(code)
            res.set_header("Content-Type", "text/html")
            res.body = body
            return res
    // Fallback: inline HTML
    res      = Response(code)
    res.body = "<html><body><h1>" + code + " " + STATUS[code] + "</h1></body></html>"
    return res
```

---

## I — CGI Handler  `src/cgi/CGI.cpp`

```
class CGI:
    int     pid
    int     pipe_in[2]    // parent writes request body → child stdin
    int     pipe_out[2]   // child stdout → parent reads response
    string  output_buf
    time_t  start_time
    bool    done

    function run(request, server, location):
        script_path = resolve_file_path(request.path, server, location)
        interpreter = location.cgi_extensions[file_extension(request.path)]

        pipe(pipe_in)
        pipe(pipe_out)
        start_time = time(NULL)

        pid = fork()
        if pid == 0:    // ── Child ──
            close(pipe_in[1]);   dup2(pipe_in[0],  STDIN_FILENO)
            close(pipe_out[0]);  dup2(pipe_out[1], STDOUT_FILENO)
            close all other fds
            setup_env(request, server, script_path)
            char* argv[] = { interpreter.c_str(), script_path.c_str(), NULL }
            execve(interpreter.c_str(), argv, environ)
            exit(1)     // execve failed

        // ── Parent ──
        close(pipe_in[0]);   close(pipe_out[1])
        fcntl(pipe_out[0], F_SETFL, O_NONBLOCK)
        // write POST body
        if request.method == "POST":
            write(pipe_in[1], request.body)
        close(pipe_in[1])
        // add pipe_out[0] to poll_fds, client enters CGI_WAITING state

    function setup_env(request, server, script_path):
        setenv("REQUEST_METHOD",  request.method)
        setenv("QUERY_STRING",    request.query_string)
        setenv("CONTENT_TYPE",    request.headers["content-type"] or "")
        setenv("CONTENT_LENGTH",  request.headers["content-length"] or "0")
        setenv("SCRIPT_NAME",     request.path)
        setenv("PATH_INFO",       request.path)
        setenv("SERVER_NAME",     server.server_names[0])
        setenv("SERVER_PORT",     port as string)
        setenv("SERVER_PROTOCOL", "HTTP/1.1")
        setenv("SCRIPT_FILENAME", script_path)

    function read_output():
        // called from poll loop when pipe_out[0] is POLLIN
        buf[4096]
        n = read(pipe_out[0], buf, 4096)
        if n > 0: output_buf += buf[0..n]
        if n == 0: done = true   // EOF → parse and build response

    function build_response() -> Response:
        // CGI output: optional headers, blank line, body
        split output_buf at first "\r\n\r\n" or "\n\n"
        parse CGI headers (Status, Content-Type, Location …)
        status = 200
        if cgi_headers has "Status": status = parse_int(cgi_headers["Status"])
        if cgi_headers has "Location": return Response(302, location)
        res = Response(status)
        res.set_header("Content-Type", cgi_headers["Content-Type"] or "text/html")
        res.body = body_part
        return res

// ── Timeout check (called every poll iteration) ─────────────
function check_cgi_timeouts(server):
    for each client with state == CGI_WAITING:
        if time(NULL) - client.cgi.start_time > 10 seconds:
            kill(client.cgi.pid, SIGKILL)
            waitpid(client.cgi.pid, NULL, WNOHANG)
            close(client.cgi.pipe_out[0])
            send error_response(504, client.server) to client
            client.state = WRITING_RESPONSE
```

---

## J — Client State Machine  `src/server/Client.cpp`

```
function read_from_client(client):
    buf[8192]
    n = recv(client.fd, buf, 8192, 0)
    if n <= 0:
        close_client(client.fd); return
    client.last_activity = time(NULL)
    client.read_buf += buf[0..n]
    client.request.feed(client.read_buf)
    if client.request.complete:
        client.state = PROCESSING
        process_request(client)

function process_request(client):
    req    = client.request
    server = match_server(req, client.listen_fd, configs)
    loc    = match_location(req.path, server)

    // Method allowed?
    if loc and loc.methods not empty and req.method not in loc.methods:
        client.write_buf = error_response(405, server).serialize()
        // ⚠ HEAD must not be blocked by missing "HEAD" in methods list
        // treat HEAD as GET for method checking
        if req.method != "HEAD": set_write(client); return

    response = NULL
    switch req.method:
        "GET"    -> response = handle_GET(req, server, loc)
        "HEAD"   -> response = handle_HEAD(req, server, loc)
        "POST"   -> response = handle_POST(req, server, loc)
        "DELETE" -> response = handle_DELETE(req, server, loc)
        default  -> response = error_response(405, server)

    if client.state == CGI_WAITING: return  // poll loop handles it
    client.write_buf = response.serialize()
    client.state     = WRITING_RESPONSE
    set_poll_out(client.fd)

function write_to_client(client):
    n = send(client.fd, client.write_buf, client.write_buf.size, 0)
    if n < 0: close_client(client.fd); return
    client.write_buf = client.write_buf.substr(n)
    if client.write_buf.empty:
        if keep_alive(client.request):
            reset_client(client)    // reuse connection
            set_poll_in(client.fd)
        else:
            close_client(client.fd)
```

---

## K — Autoindex Generator  `src/http/Methods.cpp`

```
function generate_autoindex(dir_path, uri_path) -> Response:
    entries = readdir(dir_path)      // opendir / readdir / closedir
    sort entries alphabetically
    html  = "<html><head><title>Index of " + uri_path + "</title></head><body>"
    html += "<h1>Index of " + uri_path + "</h1><hr><pre>"
    for each entry (skip "."):
        stat(entry) to get size and mtime
        html += "<a href='" + entry.name + "'>" + entry.name + "</a>"
        html += "  " + mtime + "  " + size + "\n"
    html += "</pre><hr></body></html>"
    res  = Response(200)
    res.set_header("Content-Type", "text/html")
    res.body = html
    return res
```

---

## L — main.cpp

```
int main(int argc, char* argv[]):
    if argc != 2:
        config_path = "webserv.conf"     // default
    else:
        config_path = argv[1]

    // Validate extension
    if not config_path.ends_with(".conf"):
        error("Config file must have .conf extension")

    // Tokenize
    tokens  = Tokenizer::tokenize(config_path)

    // Parse
    configs = Parser::parse(tokens)
    if configs.empty:
        error("No server blocks found in config")

    // Ignore SIGPIPE (broken pipe must not crash the server)
    signal(SIGPIPE, SIG_IGN)

    // Boot
    Server server(configs)
    server.init()
    server.run()    // never returns under normal operation
    return 0
```

---

## M — Key Edge Cases Checklist

| Scenario | Expected behaviour |
|---|---|
| `HEAD /` | Same headers as GET, zero body bytes |
| `POST /` with `Content-Length: 0` | Accept, return 200 or 204 — never 400 |
| `POST` body > `client_max_body_size` | 413 immediately, don't buffer whole body |
| Unknown method (`PATCH`, `PUT`) | 405 Method Not Allowed |
| URI with `../` traversal | Sanitize / return 400 or 403 |
| CGI script hangs > 10 s | SIGKILL child, return 504 |
| Directory without trailing `/` | 301 redirect to `path/` |
| File not readable | 403, not 404 |
| Two servers same port, different `server_name` | Route by `Host:` header |
| Keep-alive: second request on same fd | Re-use Client, reset parser state |
| `poll()` returns POLLHUP on client fd | Close without trying to read |
| `send()` returns -1 / EAGAIN | Queue remainder, retry on next POLLOUT |
| Zombie CGI processes | `waitpid(WNOHANG)` every poll iteration |
| Empty `methods` list in location | Allow ALL methods (nginx behaviour) |

---

## N — Makefile

```makefile
NAME    = webserv
CC      = c++
FLAGS   = -Wall -Wextra -Werror -std=c++98

SRCS    = src/main.cpp \
          src/config/Tokenizer.cpp \
          src/config/Parser.cpp \
          src/config/Config.cpp \
          src/server/Server.cpp \
          src/server/Client.cpp \
          src/http/Request.cpp \
          src/http/Response.cpp \
          src/http/Router.cpp \
          src/http/Methods.cpp \
          src/cgi/CGI.cpp \
          src/utils/Utils.cpp \
          src/utils/Logger.cpp

OBJS    = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CC) $(FLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
```
