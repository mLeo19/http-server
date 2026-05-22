#ifndef HANDLERS_H
#define HANDLERS_H

#include "request.h"

// example handler — returns server status as JSON
void handle_status(int fd, HttpRequest *req);

#endif