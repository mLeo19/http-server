#include <stdio.h>
#include <string.h>
#include "../include/handlers.h"
#include "../include/response.h"
#include <pthread.h>

// protects todos array from simultaneous thread access
static pthread_mutex_t todos_lock = PTHREAD_MUTEX_INITIALIZER;

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

    pthread_mutex_lock(&todos_lock);

    if (todo_count == 0) {
        snprintf(buf, sizeof(buf), "[]");
    } else {
        int offset = 0;
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
        buf[offset - 1] = ']';
        buf[offset] = '\0';
    }

    pthread_mutex_unlock(&todos_lock);

    return send_json(fd, 200, buf);
}

int handle_create_todo(int fd, HttpRequest *req) {
    if (req->content_length == 0) {
        return send_json(fd, 400,
            "{\"error\": \"body required\"}");
    }

    // extract title before locking
    char title[256] = {0};
    const char *t = strstr(req->body, "\"title\"");
    if (!t) {
        return send_json(fd, 400,
            "{\"error\": \"title required\"}");
    }
    t = strchr(t, ':');
    if (!t) {
        return send_json(fd, 400,
            "{\"error\": \"invalid format\"}");
    }
    t++;
    while (*t == ' ' || *t == '"') t++;
    int i = 0;
    while (*t && *t != '"' && i < 255) {
        title[i++] = *t++;
    }
    title[i] = '\0';

    pthread_mutex_lock(&todos_lock);

    if (todo_count >= 100) {
        pthread_mutex_unlock(&todos_lock);
        return send_json(fd, 500,
            "{\"error\": \"storage full\"}");
    }

    todos[todo_count].id   = next_id++;
    todos[todo_count].done = 0;
    strncpy(todos[todo_count].title, title, 255);
    todo_count++;

    char response[512];
    snprintf(response, sizeof(response),
        "{\"id\":%d,\"title\":\"%s\",\"done\":false}",
        todos[todo_count - 1].id,
        todos[todo_count - 1].title);

    pthread_mutex_unlock(&todos_lock);

    return send_json(fd, 201, response);
}

int handle_delete_todo(int fd, HttpRequest *req) {
    int id = -1;
    sscanf(req->query, "id=%d", &id);
    if (id == -1) {
        return send_json(fd, 400,
            "{\"error\": \"id required\"}");
    }

    pthread_mutex_lock(&todos_lock);

    int found = -1;
    for (int i = 0; i < todo_count; i++) {
        if (todos[i].id == id) {
            found = i;
            break;
        }
    }

    if (found == -1) {
        pthread_mutex_unlock(&todos_lock);
        return send_json(fd, 404,
            "{\"error\": \"todo not found\"}");
    }

    for (int i = found; i < todo_count - 1; i++) {
        todos[i] = todos[i + 1];
    }
    todo_count--;

    pthread_mutex_unlock(&todos_lock);

    return send_json(fd, 200,
        "{\"message\": \"deleted\"}");
}

int handle_toggle_todo(int fd, HttpRequest *req) {
    int id = -1;
    sscanf(req->query, "id=%d", &id);
    if (id == -1) {
        return send_json(fd, 400,
            "{\"error\": \"id required\"}");
    }

    pthread_mutex_lock(&todos_lock);

    for (int i = 0; i < todo_count; i++) {
        if (todos[i].id == id) {
            todos[i].done = !todos[i].done;

            char response[512];
            snprintf(response, sizeof(response),
                "{\"id\":%d,\"title\":\"%s\",\"done\":%s}",
                todos[i].id,
                todos[i].title,
                todos[i].done ? "true" : "false");

            pthread_mutex_unlock(&todos_lock);
            return send_json(fd, 200, response);
        }
    }

    pthread_mutex_unlock(&todos_lock);
    return send_json(fd, 404,
        "{\"error\": \"todo not found\"}");
}