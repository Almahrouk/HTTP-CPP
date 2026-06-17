/*
A] General Notes:
1. The code is written in C++ 98 standard.
2. use allowed function in subject file 
execve, pipe, strerror, gai_strerror, errno, dup,
dup2, fork, socketpair, htons, htonl, ntohs, ntohl,
select, poll, epoll (epoll_create, epoll_ctl,
epoll_wait), kqueue (kqueue, kevent), socket,
accept, listen, send, recv, chdir, bind, connect,
getaddrinfo, freeaddrinfo, setsockopt, getsockname,
getprotobyname, fcntl, close, read, write, waitpid,
kill, signal, access, stat, open, opendir, readdir
and closedir.
3. HTTP 1.0 is suggested

B] Build Project Structure and Makefile
    1. executable name: webserv 
    2. Run the code: ./webserv [configuration file]
    3. Makefile should include the following targets:
        - all: build the project
        - clean: remove object files
        - fclean: remove object files and executable
        - re: clean and rebuild the project
    4. flags: -Wall -Wextra -Werror -std=c++98
    5. the configuration file should be provided as an argument on the command line, or available in a default path

C] Tokenization and validation of configuration files.
    1. the code take only two arguments (file name & config file)
    2. check if the file name is valid and exists
        - exists (find it) & valid permission to read
        - valid extenstion (.conf)
    3. check config format and tokenize it
        - comment line (start with #) -> ignore
        - empty line -> ignore
            - valid line -> tokenize (split by space)
            - token types: 
                - simple directive (e.g., "listen 80;") -> one line, one or more parameters, ends with semicolon
                - block directive (e.g., "server { ... }") 
                    -> multi-line, 
                    starts with directive name and parameters, 
                    followed by opening brace '{', contains nested directives, 
                    ends with closing brace '}'
            - token types:
                - directive name (e.g., "listen", "server")
                - directive parameters (e.g., "80" for "listen")
            - check if the first token is a valid directive
            - check if the number of tokens is correct for that directive
            
            - token structure:
                - type (simple or block) -> enum
                - name (directive name) -> string
                - parameters (vector of strings) -> vector<token> || vector<string>
                - line number (for error reporting) -> int

D] Parsing the configuration file
    - define a class to represent the configuration file
    - listen, server_name, root, index, error_page, client_max_body_size, ... --> as enum?
    - Allowed methods, autoindex on/off, return (redirect), cgi_pass
    - Validation & errors: Invalid config must print a clear error and exit — never silently continue

E] Socket & I/O loop:
    - TCP socket setup: socket(), bind(), listen(), accept() — all set to non-blocking with fcntl()
    - poll() event loop: 
        - Single poll() call monitors all file descriptors — server fds + client fds
        - wait for events on the listening socket and client sockets
        - handle new connections, read requests, write responses
        - handle timeouts, errors, and disconnections
    - Multiple ports: 
        - listen on multiple ports as specified in the configuration file
        - each port can have its own server block with different settings
    - Client lifecycle: Accept → read → parse → respond → close/keep-alive, all non-blocking
        - accept new connection -> create a new client object -> add to poll() monitoring
        - read request from client -> parse request -> generate response -> send response
        - close connection on error or after response is sent

F] HTTP engine: Use curl -v religiously — it shows you exactly what bytes go over the wire.
    - Request parser: 
        - parse request line (method, URI, version)
        - parse headers (key-value pairs)
        - handle chunked transfer encoding and content-length
    - Response generator:
        - generate response line (version, status code, reason phrase)
        - generate headers (Content-Type, Content-Length, Connection, etc.)
        - generate body (static files, CGI output, error pages)
    - Methods: GET, POST, DELETE
        - GET: serve static files, handle autoindex, return 404 if not found
        - POST: handle form submissions, file uploads, CGI execution, enforce max body size with 413
        - DELETE: remove files, return 204 if successful, 404 if not found
    - Status codes: 200 201 204 301 302 400 403 404 405 413 500 502 — all must be correct
        - 200: OK
        - 201: Created
        - 204: No Content
        - 301: Moved Permanently
        - 302: Found
        - 400: Bad Request
        - 403: Forbidden
        - 404: Not Found
        - 405: Method Not Allowed
        - 413: Payload Too Large
        - 500: Internal Server Error
        - 502: Bad Gateway
    - Custom error pages:
        - allow user to specify custom error pages in the configuration file

G] CGI:
    - fork() + execve():
        - create a child process to execute the CGI script ( (.py, .php) per request)
    - Environment variables:
        - set the required CGI environment variables: 
            (REQUEST_METHOD, QUERY_STRING, CONTENT_TYPE, 
            CONTENT_LENGTH, SCRIPT_NAME, PATH_INFO, 
            SERVER_NAME, SERVER_PORT, etc.)
    - Handle input/output:
        - redirect stdin to the request body (for POST)
        - redirect stdout to a pipe to capture CGI output
    - Timeout & cleanup:
        - implement a timeout for CGI execution to prevent hanging (SIGKILL + waitpid()) ->  No zombie processes.
        - clean up child processes and pipes after execution
⚠ CGI timeout handling is mandatory — a hanging script must not block your server. 
Use waitpid() with WNOHANG in your poll loop.

H] Testing:
    - check listening ports:
        - ss:
            - ss -tlnp
            - ss -tlnp sport = :8080
        - netstat:
            - netstat -tlnp
            - netstat -tlnp | grep 8080
        - lsof:
            - lsof -p <pid> | grep socket
            - lsof -i :8080
            - lsof -i -P -n | grep LISTEN
        - ls, cat:
            - ls -la /proc/<pid>/fd
            - cat /proc/<pid>/net/tcp
        - watch:
            - watch -n 1 'ss -tlnp | grep webserv'
    - traffic capture:
        - tcpdump:
            - sudo tcpdump -i lo -n -v
            - sudo tcpdump -i lo -w loopback.pcap
            - sudo tcpdump -i lo port 8080 -A -n
            - Results:
                - [S]: SYN
                - [.]: ACK
                - [F]: FIN
                - [R]: RST
                - [P]: PUSH
        - wireshark:
            - sudo wireshark
            - loopback interface, filter by port, follow TCP stream
    - nc tests:
        - nc -l 8080
        - nc localhost 8080
        - nc -v localhost 8080
        - nc 127.0.0.1 8080
        - nc 127.0.0.1 8080 < request.txt
    - test files:
        - printf "GET /cgi/hello.py?q=test HTTP/1.1\r\nHost: localhost:8080\r\n\r\n" > request.txt
        - cat -A request.txt
        It need to end with: 2*(^M$ == \r\n)
    - curl tests:
        - All methods, edge cases, large bodies, bad requests, missing headers
        - curl -v to see the exact request/response bytes
        - curl -X GET localhost:8080/test.html
        - curl -X POST -d "param1=value1&param2=value2" localhost:8080/form
        - curl -X DELETE localhost:8080/file_to_delete
        - curl -X GET localhost:8080/nonexistent_file
        - curl -v --max-time 15 "http://localhost:8080/" \
            --connect-timeout 5 \
            -H "Connection: keep-alive"
        - curl -v "http://localhost:8080/cgi/hello.py?q=" \
            -H "Cookie: sessionToken=BB54AE6D76E39587" \
            -H "Connection: keep-alive" \
            -H "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/149.0.0.0 Safari/537.36" \
            -H "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*//*;q=0.8" \
            -H "Referer: http://localhost:8080/index.html" \
            -H "Accept-Language: en-US,en;q=0.9"
    - wget tests:
        - wget --spider localhost:8080/test.html
        - wget --spider localhost:8080/nonexistent_file
        - wget -O- "http://localhost:8080/cgi/hello.py?q=" \
            --header="Cookie: sessionToken=BB54AE6D76E39587" \
            --header="Referer: http://localhost:8080/index.html" \
            --header="User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)"
    - Browser test:
        - Chrome/Firefox — full page load, forms, file uploads, redirects
    - Stress test:
        - ab (ApacheBench) or siege to simulate multiple concurrent clients
        - measure response times, throughput, and error rates
        - siege or ab — no crash, no hang, no zombie CGI processes under load
    - Valgrind:
        - check for memory leaks, invalid reads/writes, and uninitialized variables
        - valgrind --leak-check=full ./webserv [configuration file]
    - Error resilience:
        - Bad config, missing files, broken pipes, signal handling — all graceful
    - Makefile rules:
        - all, clean, fclean, re — no unexpected relink, no missing deps


methods, CGI, errors, edge cases
1. RFC 7230–7235, request/response cycle, headers, status codes
2. Run nginx locally, test GET/POST/DELETE, study its config format closely
*/