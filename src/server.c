#include <stdio.h>       // printf, fprintf, perror
#include <stdlib.h>      // exit, EXIT_FAILURE
#include <string.h>      // strlen, memset
#include <unistd.h>      // close
#include <errno.h>       // errno — read after failed syscalls
#include <sys/types.h>   // pid_t and other fundamental types
#include <sys/socket.h>  // socket, bind, listen, accept, setsockopt
#include <netdb.h>       // getaddrinfo, freeaddrinfo, gai_strerror
#include <arpa/inet.h>   // inet_ntop — convert IP to human readable string
#include "../include/request.h" // parse_request function prototype
#include "../include/files.h"
#include "../include/response.h"
#include "../include/server.h"

// returns pointer to IPv4 or IPv6 address inside a sockaddr
// handles both address families so inet_ntop works correctly
static void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// allocate and initialize a new server
Server *create_server(int port) {
    Server *s = malloc(sizeof(Server));
    if (!s) {
        perror("malloc");
        return NULL;
    }
    s->port = port;
    s->sockfd = -1;
    s->route_count = 0;
    memset(s->routes, 0, sizeof(s->routes));
    return s;
}

// register a route with the server
void register_route(Server *s,
                   const char *method,
                   const char *path,
                   HandlerFn handler) {
    if (s->route_count >= MAX_ROUTES) {
        fprintf(stderr, "max routes reached\n");
        return;
    }
    strncpy(s->routes[s->route_count].method, method, 15);
    strncpy(s->routes[s->route_count].path,   path,   255);
    s->routes[s->route_count].handler = handler;
    s->route_count++;
}

// find matching route and call its handler
// if no match found → try static files → 404
static void dispatch(Server *s, int fd, HttpRequest *req) {
    // check registered routes first
    for (int i = 0; i < s->route_count; i++) {
        if (strcmp(s->routes[i].method, req->method) == 0 &&
            strcmp(s->routes[i].path,   req->path)   == 0) {
            s->routes[i].handler(fd, req);
            return;
        }
    }

    // no route matched — try serving a static file
    size_t file_size;
    char *content = read_file(req->path, &file_size);
    if (content != NULL) {
        // resolve "/" to "/index.html" for MIME detection
        // read_file already serves index.html for "/"
        // but we need the correct path for content type
        const char *mime_path = strcmp(req->path, "/") == 0 ? "/index.html" : req->path;
        send_file(fd, mime_path, content, file_size);
        free(content);
        return;
    }

    // nothing matched — 404
    send_not_found(fd);
}

// bind and listen
static int setup_socket(const char *port) {
    struct addrinfo hints, *servinfo, *p;
    int sockfd;
    int yes = 1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;     // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;   // TCP stream sockets
    hints.ai_flags    = AI_PASSIVE;    // use this machine's IP

    // getaddrinfo fills servinfo with linked list of valid addresses
    // NULL = use this machine's IP (because AI_PASSIVE is set)
    // gai_strerror converts error code to readable string
    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    // loop through results, bind to first one that works
    // getaddrinfo may return multiple addresses, try each
    for (p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family,
                       p->ai_socktype,
                       p->ai_protocol);
        if (sockfd == -1) {
            perror("socket");
            continue;
        }

        // lose the pesky "Address already in use" error message
        // without this you'd wait 30-60 seconds between restarts
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                      &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        // claim the port - fails if something else is using it
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("bind");
            continue;
        }

        break; // socket created and bound, we're done
    }

    // getaddrinfo allocated servinfo on the heap
    // we're done with it, free it now
    freeaddrinfo(servinfo);

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        return -1;
    }

    // mark socket as passive — ready to accept incoming connections
    // BACKLOG = how many connections can queue while we're busy
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        return -1;
    }

    return sockfd;
}

// main accept loop
void start_server(Server *s) {
    char port_str[8];
    snprintf(port_str, sizeof port_str, "%d", s->port);

    s->sockfd = setup_socket(port_str);
    if (s->sockfd == -1) {
        fprintf(stderr, "failed to start server\n");
        return;
    }

    printf("server: listening on port %d\n", s->port);

    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    char s_addr[INET6_ADDRSTRLEN];
    char buffer[BUFFER_SIZE];

    while (1) {
        sin_size = sizeof their_addr;

        int new_fd = accept(s->sockfd,
                           (struct sockaddr*)&their_addr,
                           &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                 get_in_addr((struct sockaddr*)&their_addr),
                 s_addr, sizeof s_addr);
        printf("server: connection from %s\n", s_addr);

        // zero buffer before reading — no leftover garbage from last request
        memset(buffer, 0, BUFFER_SIZE);

        // read raw HTTP requerst into buffer
        // BUFFER_SIZE - 1 leaves room for null terminator
        // kernel sets errno if this fails
        int bytes_read = recv(new_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_read == -1) {
            perror("recv");
            close(new_fd);
            continue;
        }

        // parse the request
        HttpRequest req;

        if (!parse_request(buffer, &req)) {
            send_bad_request(new_fd);
            close(new_fd);
            continue;
        }

        printf("Method: %s Path: %s\n", req.method, req.path);

        // dispatch to handler or static file
        dispatch(s, new_fd, &req);

        // close this client's connection
        // without this we leak file descriptors
        // eventually the OS runs out and crashes
        close(new_fd);
    }
}

// free all server resources
void free_server(Server *s) {
    if (s->sockfd != -1) {
        close(s->sockfd);
    }
    free(s);
}