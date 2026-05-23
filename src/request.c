#include <stdio.h>
#include <string.h> // strstr, memset, memcpy, sscanf
#include "../include/request.h"

// parses the request line from a raw HTTP request buffer
// fills in req->method, req->path, req->version
// returns 1 on success, 0 if request is malformed
int parse_request(const char *buffer, HttpRequest *req) {

    // zero out the struct first
    memset(req, 0, sizeof(HttpRequest));

    // 1. parse request line — method, path, version
    int result = sscanf(buffer, "%15s %255s %15s",
                        req->method,
                        req->path,
                        req->version);
    if (result != 3) return 0;

    // 2. find the end of headers
    const char *header_end = strstr(buffer, "\r\n\r\n");
    if (!header_end) return 1; // no body, still valid

    // 3. parse Content-Length from headers
    const char *cl = strstr(buffer, "Content-Length: ");
    if (cl && cl < header_end) {
        sscanf(cl, "Content-Length: %zu", &req->content_length);
    }

    // 4. parse Content-Type from headers
    const char *ct = strstr(buffer, "Content-Type: ");
    if (ct && ct < header_end) {
        sscanf(ct, "Content-Type: %127s", req->content_type);
    }

    // 5. copy body — everything after \r\n\r\n
    const char *body = header_end + 4;
    if (*body != '\0' && req->content_length > 0) {
        size_t copy_len = req->content_length;
        if (copy_len > sizeof(req->body) - 1) {
            copy_len = sizeof(req->body) - 1;
        }
        memcpy(req->body, body, copy_len);
        req->body[copy_len] = '\0';
    }

    return 1;
}