# HTTP Server in C

A production-style HTTP/1.1 server built from raw TCP sockets in C with zero external dependencies.

**Live demo:** http://52.86.19.65:8080

## Why C

Built in C to demonstrate understanding of systems programming fundamentals — manual memory management, raw socket APIs, and explicit control over concurrency. Every abstraction in modern web frameworks exists to solve problems that are visible at this level. Building an HTTP server in C makes those problems and their solutions explicit.

## Benchmarks

Tested on AWS EC2 t2.micro (Ubuntu 26.04) using Apache Bench.

| Endpoint | Requests/sec | Latency | Failed |
|----------|-------------|---------|--------|
| GET /api/status | 11,094 | 0.090ms | 0/10,000 |
| GET / (14KB HTML) | 7,852 | 0.127ms | 0/10,000 |

```bash
ab -n 10000 -c 100 http://localhost:8080/api/status
ab -n 10000 -c 10  http://localhost:8080/
```

## Features

- HTTP/1.1 request parsing — method, path, version, headers, body
- Routing system with function pointers — register any handler for any route
- Static file serving with correct MIME types
- Request body parsing — Content-Type, Content-Length, JSON body
- Query string parsing — `/api/todos?id=1` → `req->query`
- Thread pool with 8 concurrent workers using pthreads
- Mutex protected shared state — thread safe handlers
- Graceful shutdown on SIGINT/SIGTERM
- Timestamped request logging with response times
- Path traversal attack protection
- IPv4 and IPv6 support via getaddrinfo

## Project Structure

```
http-server/
├── Makefile
├── src/
│   ├── main.c          → registers routes, starts server
│   ├── server.c        → socket setup, accept loop, thread pool
│   ├── request.c       → HTTP request parsing
│   ├── response.c      → HTTP response helpers
│   ├── files.c         → static file serving
│   ├── mime.c          → MIME type detection
│   ├── handlers.c      → route handler functions
│   ├── logger.c        → timestamped request logging
│   └── threadpool.c    → thread pool implementation
├── include/
│   ├── server.h
│   ├── request.h
│   ├── response.h
│   ├── files.h
│   ├── mime.h
│   ├── handlers.h
│   ├── logger.h
│   └── threadpool.h
└── static/             → static files served at /
    ├── index.html
    └── style.css
```

## Requirements

- clang or gcc
- make
- Unix based OS (macOS or Linux)
- pthreads (included on all Unix systems)

## Build and Run

```bash
git clone https://github.com/mLeo19/http-server.git
cd http-server
make && ./server
```

Server listens on port 8080. Open your browser at:

```
http://localhost:8080
```

## How It Works

```
Browser request
      ↓
TCP socket accepts connection (main thread)
      ↓
fd handed to thread pool queue
      ↓
Worker thread picks up fd
      ↓
recv() reads raw HTTP request
      ↓
parse_request() extracts method, path, headers, body
      ↓
dispatch() matches route or serves static file
      ↓
Handler sends JSON or file response
      ↓
log_request() logs method, path, status, time
      ↓
close(fd) — connection closed
```

## Build On Top Of This

This server is designed as a framework. Clone it and add your own routes and handlers without touching the server infrastructure.

### 1. Write Your Handler

Add your handler function to `src/handlers.c`:

```c
int handle_users(int fd, HttpRequest *req) {
    // req->method  → "GET", "POST", etc.
    // req->path    → "/api/users"
    // req->body    → JSON body for POST requests
    // req->query   → "id=1" for /api/users?id=1

    send_json(fd, 200, "{\"users\": []}");
    return 200;
}
```

Declare it in `include/handlers.h`:

```c
int handle_users(int fd, HttpRequest *req);
```

### 2. Register Your Route

Add your route in `src/main.c`:

```c
int main(void) {
    Server *s = create_server(8080);

    // your routes
    register_route(s, "GET",    "/api/users",  handle_users);
    register_route(s, "POST",   "/api/users",  handle_create_user);
    register_route(s, "DELETE", "/api/users",  handle_delete_user);

    start_server(s);
    free_server(s);
    return 0;
}
```

### 3. Add Your Frontend

Drop your HTML, CSS, and JS into the `static/` folder:

```
static/
├── index.html
├── style.css
└── app.js
```

Your JavaScript can fetch from your API:

```javascript
fetch('/api/users')
    .then(r => r.json())
    .then(data => console.log(data));
```

### 4. Build and Run

```bash
make && ./server
```

### Response Helpers

Use these in your handlers — no need to format raw HTTP:

```c
send_json(fd, 200, "{\"status\": \"ok\"}");      // JSON response
send_json(fd, 201, "{\"id\": 1}");               // 201 Created
send_json(fd, 404, "{\"error\": \"not found\"}"); // 404
send_html(fd, 200, "<h1>Hello</h1>");             // HTML response
send_not_found(fd);                               // 404 Not Found
send_bad_request(fd);                             // 400 Bad Request
```

### Adding a Database

Handlers are plain C functions — add any database library you want:

```c
// SQLite example
#include <sqlite3.h>

int handle_get_todos(int fd, HttpRequest *req) {
    sqlite3 *db;
    sqlite3_open("todos.db", &db);
    // query and build JSON response
    sqlite3_close(db);
    return send_json(fd, 200, json_result);
}
```

Link with `-lsqlite3` in your Makefile.

## Built-in Demo Endpoints

```
GET  /api/status        → server status JSON
POST /api/echo          → echoes request body back
GET  /api/todos         → list all todos
POST /api/todos         → create a todo {"title": "..."}
PATCH  /api/todos?id=1  → toggle todo done status
DELETE /api/todos?id=1  → delete a todo
```

## Deploy on EC2

```bash
# on your EC2 instance
sudo apt install -y gcc make git clang
git clone https://github.com/mLeo19/http-server.git
cd http-server
make

# run in background
nohup ./server > server.log 2>&1 &

# watch logs
tail -f server.log

# stop
kill $(pgrep server)
```

Open port 8080 in your EC2 security group inbound rules.

## Security

- Path traversal attacks blocked — requests containing `..` are rejected
- Buffer overflow protection — all string operations use size-limited variants
- Thread safe handlers — shared state protected by mutex
- Address sanitizer enabled in debug builds

## Concurrency

The server runs 8 worker threads simultaneously via a thread pool.
Handlers are called concurrently — multiple requests are processed
at the same time by different threads.

### Thread Safety Rules

**Local variables are always safe:**
```c
int handle_example(int fd, HttpRequest *req) {
    char buf[256];  // lives on this thread's stack
    int count = 0;  // lives on this thread's stack
    // no mutex needed — each thread has its own stack
}
```

**Shared state requires a mutex:**
```c
// static and global variables are shared across all threads
static int counter = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int handle_counter(int fd, HttpRequest *req) {
    pthread_mutex_lock(&lock);
    counter++;                    // now thread safe
    int val = counter;
    pthread_mutex_unlock(&lock);

    char json[64];
    snprintf(json, sizeof json, "{\"count\": %d}", val);
    return send_json(fd, 200, json);
}
```

### Why Local Variables Are Safe

Each thread has its own private stack. When two threads call the
same handler simultaneously they each get completely independent
copies of all local variables at different memory addresses.
They never touch each other's memory.

Only static variables, global variables, and heap allocated data
shared between threads require mutex protection. See `handlers.c`
for a complete example using `pthread_mutex_t` to protect the
in-memory todo store.

## Roadmap

- HTTPS via OpenSSL
- HTTP keep-alive connections
- Dynamic route parameters (`/api/users/:id`)
- epoll event loop for higher concurrency
- WebSocket support
- Rate limiting

## Author

Built by [Leonardo Maicelo](https://github.com/mLeo19)

- GitHub: [github.com/mLeo19/http-server](https://github.com/mLeo19/http-server)
- Live: [http://52.86.19.65:8080](http://52.86.19.65:8080)