#include <stdio.h>
#include <string.h>
#include "../include/handlers.h"
#include "../include/response.h"

// ─── in memory todo store ───────────────────────────────
typedef struct {
    int id;
    char title[256];
    int done; // 0 = false, 1 = true
} Todo;

static Todo todos[100];
static int todo_count = 0;
static int next_id = 1;
// ────────────────────────────────────────────────────────

int handle_status(int fd, HttpRequest *req) {
    (void)req;
    send_json(fd, 200, "{\"status\": \"running\"}");
    return 200;
}

int handle_echo(int fd, HttpRequest *req) {
    if (req->content_length > 0) {
        send_json(fd, 200, req->body);
        return 200;
    } else {
        send_json(fd, 400, "{\"error\": \"no body\"}");
        return 400;
    }
}

int handle_get_todos(int fd, HttpRequest *req) {
    (void)req;
    char buf[4096];
    int offset = 0;

    if (todo_count == 0) {
        snprintf(buf, sizeof(buf), "[]");
    } else {
        offset += snprintf(buf + offset,
                          sizeof(buf) - offset, "[");
        for (int i = 0; i < todo_count; i++) {
            offset += snprintf(buf + offset,
                              sizeof(buf) - offset,
                "{\"id\":%d,\"title\":\"%s\",\"done\":%s},",
                todos[i].id,
                todos[i].title,
                todos[i].done ? "true" : "false");
        }
        // replace trailing comma with closing bracket
        buf[offset - 1] = ']';
        buf[offset] = '\0';
    }

    send_json(fd, 200, buf);
    return 200;
}

int handle_create_todo(int fd, HttpRequest *req) {
    // check body exists
    if (req->content_length == 0) {
        send_json(fd, 400, "{\"error\": \"body required\"}");
        return 400;
    }

    // check store isn't full
    if (todo_count >= 100) {
        send_json(fd, 500, "{\"error\": \"storage full\"}");
        return 500;
    }

    // extract title from body
    // body looks like: {"title": "buy milk"}
    char title[256] = {0};
    const char *t = strstr(req->body, "\"title\"");
    if (!t) {
        send_json(fd, 400, "{\"error\": \"title required\"}");
        return 400;
    }

    // skip past "title"
    t = strchr(t, ':');
    if (!t) {
        send_json(fd, 400, "{\"error\": \"invalid format\"}");
        return 400;
    }
    t++; // skip ':'

    // skip whitespace and opening quote
    while (*t == ' ' || *t == '"') t++;

    // copy until closing quote
    int i = 0;
    while (*t && *t != '"' && i < 255) {
        title[i++] = *t++;
    }
    title[i] = '\0';

    // add to store
    todos[todo_count].id   = next_id++;
    todos[todo_count].done = 0;
    strncpy(todos[todo_count].title, title, 255);
    todo_count++;

    // send 201 Created with new todo
    char response[512];
    snprintf(response, sizeof(response),
        "{\"id\":%d,\"title\":\"%s\",\"done\":false}",
        todos[todo_count - 1].id,
        todos[todo_count - 1].title);
    send_json(fd, 201, response);
    return 201;
}

int handle_delete_todo(int fd, HttpRequest *req) {
    // get id from query string
    // query looks like: "id=1"
    int id = -1;
    sscanf(req->query, "id=%d", &id);
    if (id == -1) {
        send_json(fd, 400, "{\"error\": \"id required\"}");
        return 400;
    }

    // find todo with that id
    int found = -1;
    for (int i = 0; i < todo_count; i++) {
        if (todos[i].id == id) {
            found = i;
            break;
        }
    }

    if (found == -1) {
        send_json(fd, 404, "{\"error\": \"todo not found\"}");
        return 404;
    }

    // remove by shifting remaining todos down
    for (int i = found; i < todo_count - 1; i++) {
        todos[i] = todos[i + 1];
    }
    todo_count--;

    send_json(fd, 200, "{\"message\": \"deleted\"}");
    return 200;
}

int handle_toggle_todo(int fd, HttpRequest *req) {
    // get id from query string
    int id = -1;
    sscanf(req->query, "id=%d", &id);
    if (id == -1) {
        send_json(fd, 400, "{\"error\": \"id required\"}");
        return 400;
    }

    // find todo with that id
    for (int i = 0; i < todo_count; i++) {
        if (todos[i].id == id) {
            // flip done status
            todos[i].done = !todos[i].done;

            // send updated todo
            char response[512];
            snprintf(response, sizeof(response),
                "{\"id\":%d,\"title\":\"%s\",\"done\":%s}",
                todos[i].id,
                todos[i].title,
                todos[i].done ? "true" : "false");
            send_json(fd, 200, response);
            return 200;
        }
    }

    send_json(fd, 404, "{\"error\": \"todo not found\"}");
    return 404;
}