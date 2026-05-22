#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include "../include/mime.h"
#include "../include/response.h"

// note: send_json and send_html are for 200 OK responses only
// error responses have their own dedicated functions below

void send_json(int fd, int status, const char *body) {
    char response_header[256];
    snprintf(response_header, sizeof response_header,
        "HTTP/1.1 %d OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, strlen(body));
    if (send(fd, response_header, strlen(response_header), 0) == -1) {
        perror("send");
    }
    if (send(fd, body, strlen(body), 0) == -1) {
        perror("send");
    }
}

void send_html(int fd, int status, const char *body) {
    char response_header[256];
    snprintf(response_header, sizeof response_header,
        "HTTP/1.1 %d OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, strlen(body));
    if (send(fd, response_header, strlen(response_header), 0) == -1) {
        perror("send");
    }
    if (send(fd, body, strlen(body), 0) == -1) {
        perror("send");
    }
}

void send_file(int fd, const char *path,
               const char *content, size_t file_size) {
    // determine content type from file extension
    const char *content_type = map_to_content_type(path);
    char response_header[256];
    snprintf(response_header, sizeof response_header,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        content_type, file_size);
    if (send(fd, response_header, strlen(response_header), 0) == -1) {
        perror("send");
    }
    if (send(fd, content, file_size, 0) == -1) {
        perror("send");
    }
}

void send_not_found(int fd) {
    char *body = "Not Found";
    char response_header[256];
    snprintf(response_header, sizeof response_header,
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        strlen(body));
    if (send(fd, response_header, strlen(response_header), 0) == -1) {
        perror("send");
    }
    if (send(fd, body, strlen(body), 0) == -1) {
        perror("send");
    }
}

void send_bad_request(int fd) {
    char *body = "Bad Request";
    char response_header[256];
    snprintf(response_header, sizeof response_header,
        "HTTP/1.1 400 Bad Request\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        strlen(body));
    if (send(fd, response_header, strlen(response_header), 0) == -1) {
        perror("send");
    }
    if (send(fd, body, strlen(body), 0) == -1) {
        perror("send");
    }
}