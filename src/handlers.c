#include "../include/handlers.h"
#include "../include/response.h"

void handle_status(int fd, HttpRequest *req) {
    (void)req; // unused parameter
    send_json(fd, 200, "{\"status\": \"running\"}");
}