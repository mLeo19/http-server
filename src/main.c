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
#include "../include/mime.h"

#define PORT "8080"
#define BACKLOG 10 // max pending connections in queue
#define BUFFER_SIZE 8192

// returns pointer to IPv4 or IPv6 address inside a sockaddr
// handles both address families so inet_ntop works correctly
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void) {
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // big enough for IPv4 or IPv6
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN]; // holds client IP string
    char buffer[BUFFER_SIZE];
    int yes = 1;
    int rv;

    // zero out hints before filling it in - always do this
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // use this machine's IP

    // getaddrinfo fills servinfo with linked list of valid addresses
    // NULL = use this machine's IP (because AI_PASSIVE is set)
    // gai_strerror converts error code to readable string
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through results, bind to first one that works
    // getaddrinfo may return multiple addresses, try each
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue; // try next address
        }

        // lose the pesky "Address already in use" error message
        // without this you'd wait 30-60 seconds between restarts
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        // claim the port - fails if something else is using it
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue; // try next address
        }

        break; // socket created and bound, we're done
    }

    // getaddrinfo allocated servinfo on the heap
    // we're done with it, free it now
    freeaddrinfo(servinfo);

    // if p is NULL we never successfully bound
    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        return 1;
    }

    // mark socket as passive — ready to accept incoming connections
    // BACKLOG = how many connections can queue while we're busy
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connections on port %s\n", PORT);

    // main accept loop - runs forever, one client per iteration
    while(1) {
        sin_size = sizeof their_addr;

        // blocks here until a client connects
        // returns new fd just for this client
        // sockfd keeps listening for more connections
        new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue; // keep server running on failed accept
        }

        // convert client's IP to human readable string and print it
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr), s, sizeof s);
        printf("server: got connection from %s\n", s);

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

        printf("Received %d bytes:\n%s\n", bytes_read, buffer);

        // parse the client's HTTP request
        HttpRequest req;

        // check if 3 items were successfully parsed (method, path, version)
        if (!parse_request(buffer, &req)) {
            // malformed request, send 400 Bad Request response
            char *bad_request_response =
                "HTTP/1.1 400 Bad Request\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 11\r\n"
                "Connection: close\r\n"
                "\r\n"
                "Bad Request";
            if (send(new_fd, bad_request_response, strlen(bad_request_response), 0) == -1) {
                perror("send");
            }
            close(new_fd);
            continue;
        }

        printf("Method: %s\n", req.method);
        printf("Path:   %s\n", req.path);
        printf("Version: %s\n", req.version);

        // read the requested file from disk
        size_t file_size;
        char *file_contents = read_file(req.path, &file_size);
        if (file_contents == NULL) {
            // file not found, send 404 Not Found response
            char *not_found_response =
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 9\r\n"
                "Connection: close\r\n"
                "\r\n"
                "Not Found";
            if (send(new_fd, not_found_response, strlen(not_found_response), 0) == -1) {
                perror("send");
            }
            close(new_fd);
            continue;
        }

        // file found, send 200 OK response with file contents as body
        // map extension to content type for Content-Type header
        const char *resolved_path;
        if (strcmp(req.path, "/") == 0) {
            resolved_path = "index.html";
        } else {
            resolved_path = req.path;
        }
        const char *content_type = map_to_content_type(resolved_path);
        char response_header[256];
        snprintf(response_header, sizeof response_header,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n", content_type, file_size);
        if (send(new_fd, response_header, strlen(response_header), 0) == -1) {
            perror("send");
            free(file_contents);
            close(new_fd);
            continue;
        }
        if (send(new_fd, file_contents, file_size, 0) == -1) {
            perror("send");
        }
        free(file_contents);

        // close this client's connection
        // without this we leak file descriptors
        // eventually the OS runs out and crashes
        close(new_fd);
    }

    return 0;
}