#ifndef REQUEST_H
#define REQUEST_H

#include <stddef.h> // size_t

// holds everything we parse from an HTTP request
typedef struct {
    char method[16];
    char path[256];
    char version[16];
    char query[256];       // everything after ? in the path
    char content_type[128]; // Content-Type header value
    size_t content_length;  // Content-Length header value
    char body[4096];        // request body (POST, PUT, PATCH)
} HttpRequest;

// parses raw buffer into an HttpRequest struct
// returns 1 on success, 0 on failure
int parse_request(const char *buffer, HttpRequest *req);

#endif