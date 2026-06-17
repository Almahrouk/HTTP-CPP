*This project has been created as part of the 42 curriculum by akharkho, mtahalla, ycharkou.*

## Description

WebServ is a custom HTTP/1.0 server written entirely in C++98. The primary goal of this project is to build a robust, fully functional web server from scratch. By doing so, we gain a profound understanding of the HTTP protocol, network socket programming, and how the World Wide Web operates under the hood.

Unlike modern web frameworks that abstract away the complexity of networking, WebServ requires manual handling of every aspect of client-server communication. The server relies on non-blocking I/O and multiplexing techniques via the `poll()` system call. This architecture ensures that the server can handle multiple concurrent client connections simultaneously without ever blocking or hanging. 

From parsing the configuration file to routing requests, managing file uploads, serving static assets, and executing dynamic Common Gateway Interface (CGI) scripts, WebServ acts as a lightweight but highly capable HTTP server.

## Features

Our implementation strictly adheres to the requirements of the 42 curriculum and includes the following features:

* **Non-blocking I/O**: The server is entirely non-blocking and relies on a single `poll()` loop to monitor all read and write operations across all connected sockets.
* **HTTP Methods**: Full support for GET, POST, and DELETE methods.
* **Static File Serving**: Serves complete static websites, including HTML, CSS, JavaScript, and images, automatically resolving MIME types based on file extensions.
* **CGI Execution**: Dynamic execution of CGI scripts. The server safely handles script execution using `fork()` and `execve()`, passing data through standard input/output pipes and properly configuring CGI environment variables (Meta-Variables).
* **Timeout & Zombie Handling**: Robust timeout mechanisms to prevent the server from crashing or hanging if a CGI script enters an infinite loop.
* **File Management**: Dedicated routes for uploading files and deleting files from the server storage.
* **Custom Error Pages**: Support for defining default error pages (e.g., 404 Not Found, 403 Forbidden, 500 Internal Server Error, 504 Gateway Timeout) within the configuration file.
* **Directory Listing**: Autoindex feature that generates HTML directory listings when a default index file is not found.
* **HTTP Redirection**: Supports configuring HTTP 301/308 redirects.
* **Session and Cookie Management**: Implements basic HTTP cookie handling to demonstrate session state management.

## Technical Choices

To comply with the project constraints and ensure maximum resilience, several technical choices were made:
* **C++98 Standard**: The codebase is strictly written in C++98, ensuring compatibility and relying on manual memory management and classic C++ paradigms.
* **Multiplexing with poll()**: We chose `poll()` over `select()` to manage file descriptors efficiently. It monitors the server listening socket, active client connections, and CGI pipes simultaneously.
* **State Machine**: Request parsing and response generation are managed via state machines, ensuring that partial reads (incomplete HTTP requests or large payloads) do not block the execution flow.

## Configuration File

The server is highly customizable through a configuration file, taking inspiration from NGINX semantics. You can define multiple virtual servers, each listening on different ports or hostnames.

Example configuration semantics supported:
* `listen` and `host` to bind the server.
* `server_name` to handle virtual hosting.
* `client_max_body_size` to restrict large payloads.
* `error_page` to map HTTP status codes to custom HTML files.
* `location` blocks to specify routing rules, including accepted methods, root directories, CGI extensions, and upload storage paths.

## Instructions

### Compilation
The repository includes a Makefile. To compile the source code, simply run the following command at the root of the repository:

```bash
make
```

This command will generate the `webserv` executable. You can also use `make clean` to remove object files, `make fclean` to remove both object files and the executable, and `make re` to recompile from scratch.

### Execution
To launch the server, execute the binary and provide the path to a valid configuration file.

```bash
./webserv server.conf
```

Once launched, the server will output its status and indicate which ports it is currently listening on.

### Testing with Curl
You can interact with the server using standard web browsers (like Chrome or Firefox) or by using `curl` from your terminal. Assuming your configuration binds a server to port 4242, here are some commands to test its capabilities:

1. **Send a standard GET request**:
```bash
curl -v http://localhost:4242/
```

2. **Test a CGI script with a GET request**:
```bash
curl -v "http://localhost:4242/cgi/hello.py?q=test"
```

3. **Test a CGI script with a POST request**:
```bash
curl -v -X POST \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "name=adnane&email=adnane@1337.com" \
  http://localhost:4242/cgi/hello_post.py
```

4. **Upload a file using POST**:
```bash
curl -v -X POST \
  -F "file=@/path/to/your/local/file.txt" \
  http://localhost:4242/uploads
```

5. **Delete an uploaded file**:
```bash
curl -v -X DELETE http://localhost:4242/uploads/file.txt
```

## Resources

The following classic references and documentation were used to build and understand the HTTP server:

* **HTTP RFCs**: Used as the core reference to understand HTTP/1.0 protocol standards, status codes, and proper header formatting.
* **NGINX Documentation**: Used as an architectural inspiration for designing the server configuration file semantics and comparing expected HTTP behaviors during edge cases.
* **Video Tutorials**:
  * How an HTTP Request Gets Served : https://www.youtube.com/watch?v=hWyBeEF3CqQ
  * Web Server Concepts : https://www.youtube.com/watch?v=9J1nJOivdyw&t=43s

**AI Usage**: AI was used purely as an educational learning tool to gather more information about project concepts, clarify complex networking behaviors, and break down the intricacies of the HTTP protocol specifications.
