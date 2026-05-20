#ifndef REQUEST_H
#define REQUEST_H

// holds everything we parse from an HTTP request
typedef struct {
    char method[16];
    char path[256];
    char version[16];
} HttpRequest;

// parses raw buffer into an HttpRequest struct
// returns 1 on success, 0 on failure
int parse_request(const char *buffer, HttpRequest *req);

#endif