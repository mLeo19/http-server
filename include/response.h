#ifndef RESPONSE_H
#define RESPONSE_H

#include <stddef.h> // size_t

void send_json(int fd, int status, const char *body);
void send_html(int fd, int status, const char *body);
void send_file(int fd, const char *path,
               const char *content, size_t file_size);
void send_not_found(int fd);
void send_bad_request(int fd);

#endif