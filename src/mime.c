#include <string.h>
#include <stddef.h>
#include "../include/mime.h"

const char *map_to_content_type(const char *path) {
    // find the last '.' in the path
    char *ext = strrchr(path, '.');

    // if no extension found → default to plain text
    if (ext == NULL) {
        return "text/plain";
    }

    // compare ext against known extensions
    if (strcmp(ext, ".html") == 0) {
        return "text/html";
    } else if (strcmp(ext, ".css") == 0) {
        return "text/css";
    } else if (strcmp(ext, ".js") == 0) {
        return "application/javascript";
    } else if (strcmp(ext, ".png") == 0) {
        return "image/png";
    } else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) {
        return "image/jpeg";
    } else if (strcmp(ext, ".gif") == 0) {
        return "image/gif";
    } else if (strcmp(ext, ".ico") == 0) {
        return "image/x-icon";
    } else if (strcmp(ext, ".json") == 0) {
        return "application/json";
    } else if (strcmp(ext, ".svg") == 0) {
        return "image/svg+xml";
    } else {
        // default to plain text if ext is .txt or unknown
        return "text/plain";
    }
}