#include "../include/server.h"
#include "../include/handlers.h"

int main(void) {
    // create server on port 8080
    Server *s = create_server(8080);
    if (!s) return 1;

    // register routes
    // add your own handlers here
    register_route(s, "GET", "/api/status", handle_status);
    register_route(s, "POST", "/api/echo", handle_echo);

    // todo API
    register_route(s, "GET",    "/api/todos",   handle_get_todos);
    register_route(s, "POST",   "/api/todos",   handle_create_todo);
    register_route(s, "DELETE", "/api/todos",   handle_delete_todo);
    register_route(s, "PATCH",  "/api/todos",   handle_toggle_todo);

    // start server — blocks forever
    start_server(s);

    // never reached but clean up anyway
    free_server(s);
    return 0;
}