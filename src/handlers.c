#include "../include/handlers.h"
#include "../include/response.h"

void handle_status(int fd, HttpRequest *req) {
    (void)req; // unused parameter
    send_json(fd, 200, "{\"status\": \"running\"}");
}

void handle_echo(int fd, HttpRequest *req) {
    (void)req;
    // echo the request body back as JSON response
    // proves body parsing is working
    if (req->content_length > 0) {
        send_json(fd, 200, req->body);
    } else {
        send_json(fd, 400, "{\"error\": \"no body\"}");
    }
}