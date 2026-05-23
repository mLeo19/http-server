#ifndef HANDLERS_H
#define HANDLERS_H

#include "request.h"

// example handler — returns server status as JSON
int handle_status(int fd, HttpRequest *req);

int handle_echo(int fd, HttpRequest *req);

// todo API handlers
int handle_get_todos(int fd, HttpRequest *req);
int handle_create_todo(int fd, HttpRequest *req);
int handle_delete_todo(int fd, HttpRequest *req);
int handle_toggle_todo(int fd, HttpRequest *req);

#endif