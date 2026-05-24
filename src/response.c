#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include "../include/mime.h"
#include "../include/response.h"

// returns the HTTP reason phrase for a status code
static const char *reason_phrase(int status) {
    switch (status) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        default:  return "OK";
    }
}

// note: send_json and send_html are for 200 OK responses only
// error responses have their own dedicated functions below

int send_json(int fd, int status, const char *body) {
    char response_header[256];
    snprintf(response_header, sizeof response_header,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, reason_phrase(status), strlen(body));
    if (send(fd, response_header,
             strlen(response_header), 0) == -1) {
        perror("send");
    }
    if (send(fd, body, strlen(body), 0) == -1) {
        perror("send");
    }
    return status;
}

int send_html(int fd, int status, const char *body) {
    char response_header[256];
    snprintf(response_header, sizeof response_header,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, reason_phrase(status), strlen(body));
    if (send(fd, response_header,
             strlen(response_header), 0) == -1) {
        perror("send");
    }
    if (send(fd, body, strlen(body), 0) == -1) {
        perror("send");
    }
    return status;
}

int send_file(int fd, const char *path,
              const char *content, size_t file_size) {
    const char *content_type = map_to_content_type(path);
    char response_header[256];
    snprintf(response_header, sizeof response_header,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        content_type, file_size);
    if (send(fd, response_header,
             strlen(response_header), 0) == -1) {
        perror("send");
    }
    if (send(fd, content, file_size, 0) == -1) {
        perror("send");
    }
    return 200;
}

int send_not_found(int fd) {
    char *body = "Not Found";
    char response_header[256];
    snprintf(response_header, sizeof response_header,
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        strlen(body));
    if (send(fd, response_header,
             strlen(response_header), 0) == -1) {
        perror("send");
    }
    if (send(fd, body, strlen(body), 0) == -1) {
        perror("send");
    }
    return 404;
}

int send_bad_request(int fd) {
    char *body = "Bad Request";
    char response_header[256];
    snprintf(response_header, sizeof response_header,
        "HTTP/1.1 400 Bad Request\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        strlen(body));
    if (send(fd, response_header,
             strlen(response_header), 0) == -1) {
        perror("send");
    }
    if (send(fd, body, strlen(body), 0) == -1) {
        perror("send");
    }
    return 400;
}