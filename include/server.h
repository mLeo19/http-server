#ifndef SERVER_H
#define SERVER_H

#include "request.h"  // HttpRequest

#define MAX_ROUTES 64
#define PORT "8080"
#define BACKLOG 10
#define BUFFER_SIZE 8192

// function pointer type for route handlers
// every handler takes a client fd and parsed request
typedef void (*HandlerFn)(int fd, HttpRequest *req);

// a single route — method + path + handler
typedef struct {
    char method[16];
    char path[256];
    HandlerFn handler;
} Route;

// the server — all state lives here
// no global variables
typedef struct {
    int sockfd;
    int port;
    Route routes[MAX_ROUTES];
    int route_count;
} Server;

// lifecycle functions
Server *create_server(int port);
void   start_server(Server *s);
void   free_server(Server *s);

// routing
void register_route(Server *s,
                   const char *method,
                   const char *path,
                   HandlerFn handler);

#endif