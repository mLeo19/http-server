#ifndef RESPONSE_H
#define RESPONSE_H

#include <stddef.h> // size_t

int send_json(int fd, int status, const char *body);
int send_html(int fd, int status, const char *body);
int send_file(int fd, const char *path,
               const char *content, size_t file_size);
int send_not_found(int fd);
int send_bad_request(int fd);

#endif