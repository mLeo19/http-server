# HTTP Server in C

A production-style HTTP/1.1 server built from raw TCP sockets in C with zero external dependencies.

## Features

- HTTP/1.1 request parsing (method, path, version)
- Static file serving from disk
- Correct MIME types for HTML, CSS, JS, images, JSON
- 404 and 400 error handling
- IPv4 and IPv6 support via getaddrinfo
- Connection: close header for proper browser handling

## Project Structure

```
http-server/
├── Makefile
├── src/
│   ├── main.c       → server core, accept loop
│   ├── request.c    → HTTP request parsing
│   ├── files.c      → file reading from disk
│   └── mime.c       → MIME type detection
├── include/
│   ├── request.h
│   ├── files.h
│   └── mime.h
└── static/
    ├── index.html   → served at /
    └── style.css    → stylesheet
```

## Requirements

- clang or gcc
- make
- Unix based OS (macOS or Linux)

## Build and Run

```bash
make && ./server
```

Server listens on port 8080. Open your browser at:

```
http://localhost:8080
```

## How It Works

The server follows the standard BSD sockets pattern:

1. `getaddrinfo` resolves the address and port
2. `socket` creates a TCP socket
3. `bind` claims port 8080
4. `listen` marks the socket as passive
5. `accept` blocks until a client connects
6. `recv` reads the raw HTTP request into a buffer
7. Request line is parsed with `sscanf` into method, path, version
8. Requested file is read from the `static/` directory using `stat` and `fread`
9. Correct `Content-Type` is determined from the file extension
10. Response headers and file contents are sent back with `send`
11. Connection is closed and the loop repeats

## HTTP Request Parsing

Parses the request line from the raw buffer:

```
GET /index.html HTTP/1.1
→ method:  GET
→ path:    /index.html
→ version: HTTP/1.1
```

Returns a 400 Bad Request if the request is malformed.

## File Serving

Files are served from the `static/` directory. Requesting `/` automatically serves `static/index.html`. Missing files return a 404 Not Found response.

## MIME Types

| Extension | Content-Type |
|-----------|-------------|
| .html | text/html |
| .css | text/css |
| .js | application/javascript |
| .png | image/png |
| .jpg | image/jpeg |
| .ico | image/x-icon |
| .json | application/json |
| .txt | text/plain |

## What I Learned

- BSD sockets API — socket, bind, listen, accept, send, recv
- HTTP/1.1 request and response format at the byte level
- Manual memory management in C — malloc, free, avoiding leaks
- MIME types and Content-Type headers
- Separating concerns across multiple C source files
- Deploying a C server on AWS EC2

## Roadmap

- [ ] Concurrent connections via pthreads
- [ ] HTTP keep-alive support
- [ ] JSON API endpoint
- [ ] Custom 404 page
- [ ] HTTPS via OpenSSL