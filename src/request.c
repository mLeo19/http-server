#include <stdio.h>
#include "../include/request.h"

// parses the request line from a raw HTTP request buffer
// fills in req->method, req->path, req->version
// returns 1 on success, 0 if request is malformed
int parse_request(const char *buffer, HttpRequest *req) {
    int result = sscanf(buffer, "%15s %255s %15s",
                        req->method,
                        req->path,
                        req->version);
    return result == 3;
}