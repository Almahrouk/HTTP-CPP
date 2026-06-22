/*
I've read the subject carefully. Here's a complete breakdown:

---

## Socket & Poll — All Cases to Handle

### Poll Setup
* Use **only 1 poll()** for ALL I/O — listening socket + all client sockets together
* Monitor **both POLLIN and POLLOUT** simultaneously on every fd
* Never call `read`/`recv`/`write`/`send` without poll() saying it's ready first
* Never check `errno` after a read/write to adjust behavior — **grade = 0**
* Regular disk files are exempt from poll (read/write them directly)

### Listening Socket Cases
* `POLLIN` on server fd → new client → `accept()` → add to poll list
* `accept()` fails → log, don't crash, continue
* Server fd error (`POLLERR`/`POLLNVAL`) → handle gracefully, never crash

### Client Socket Cases
* `POLLIN` on client fd → data ready → `recv()`
* `POLLOUT` on client fd → ready to send → `send()` pending response
* `POLLHUP` → client closed connection → close fd, remove from list
* `POLLERR` → error on client fd → close fd, remove from list
* `POLLNVAL` → invalid fd → remove from list
* `recv()` returns 0 → client disconnected cleanly → close + remove
* `recv()` returns -1 → error → close + remove (don't check errno)
* `send()` returns -1 → error → close + remove
* `send()` sends partial data → track bytes sent, send rest on next POLLOUT

### Non-blocking Requirements
* All sockets must be set to **non-blocking** (`fcntl(fd, F_SETFL, O_NONBLOCK)`)
* On Linux this is the approach; on MacOS `fcntl` is allowed with `F_SETFL`, `O_NONBLOCK`, `FD_CLOEXEC` only
* A request must **never hang indefinitely** — you need timeouts

### Timeout & Resilience
* Detect stalled/slow clients (no data for too long) → close them
* Server must survive stress tests and stay available at all times
* Must handle running out of memory without crashing

### Multiple Ports
* Multiple listening sockets (one per `interface:port` in config)
* All listening sockets go into the same poll list
* Each server block in config can have its own port

---

## HTTP Requests — All Cases

### Method Cases
* `GET` — serve file or directory
* `POST` — receive body, handle file upload, run CGI
* `DELETE` — delete a file
* Any other method → `405 Method Not Allowed`
* Method not allowed for that specific route → `405`

### Request Parsing Cases
* Malformed request line → `400 Bad Request`
* HTTP version not supported (not 1.0/1.1) → `505 HTTP Version Not Supported`
* Missing `Host` header (HTTP/1.1) → `400`
* Headers too large → `431 Request Header Fields Too Large`
* Body exceeds `max_body_size` from config → `413 Content Too Large`
* Chunked transfer encoding → must **un-chunk** before passing to CGI
* `Content-Length` mismatch → `400`
* Incomplete request (client sends partial data) → buffer and wait for more via poll

### URL / Routing Cases
* URL matches a location block → apply that block's rules
* URL has no matching location → `404 Not Found`
* URL path traversal (`../`) → `403 Forbidden` or sanitize
* URL maps to a file → serve it
* URL maps to a directory:
  * Has index file → serve it
  * Directory listing enabled → list directory
  * Directory listing disabled, no index → `403 Forbidden`
* HTTP redirect defined in location → `301`/`302` with `Location` header
* Requested method not in location's allowed methods list → `405`

### File Serving Cases
* File exists and readable → `200 OK` + correct `Content-Type`
* File not found → `404`
* File exists but not readable (no permission) → `403 Forbidden`
* Binary vs text files → correct MIME type
* Static website serving must work fully

### Upload Cases
* POST with file upload → save to configured upload directory
* Upload directory not writable → `500` or `403`
* Missing `Content-Type: multipart/form-data` → `400`

### CGI Cases
* File extension matches CGI (e.g. `.php`, `.py`) → execute via `execve` + `fork`
* `fork` is only allowed for CGI — nowhere else
* CGI must run in the correct directory (for relative paths)
* Full request + all headers must be passed as environment variables
* CGI returns `content_length` → use it
* CGI returns no `content_length` → read until EOF
* CGI timeout / crash → `500 Internal Server Error`
* Chunked request to CGI → un-chunk first, send as plain body with EOF

---

## HTTP Responses — All Cases

### Success
* `200 OK` — normal response
* `201 Created` — file upload successful
* `204 No Content` — DELETE success with no body
* `301`/`302` — redirect (from config)

### Client Errors
* `400 Bad Request` — malformed request
* `403 Forbidden` — no permission
* `404 Not Found` — resource missing
* `405 Method Not Allowed` — method blocked for this route
* `413 Content Too Large` — body exceeds max size
* `431 Request Header Fields Too Large`

### Server Errors
* `500 Internal Server Error` — CGI crash, unexpected failure
* `505 HTTP Version Not Supported`

### Response Requirements
* Status codes must be **accurate** (compared against Nginx behavior)
* Must have **default error pages** if none are configured
* Must set correct headers: `Content-Type`, `Content-Length`, `Connection`
* Response must never be left incomplete or hanging

---

## Quick Critical Rules Summary

```
⚠️  Only 1 poll() for everything
⚠️  Never read/write without poll() readiness check first  
⚠️  Never check errno after read/write
⚠️  fork() only for CGI
⚠️  Server must NEVER crash
⚠️  All sockets non-blocking
```
*/