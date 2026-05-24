#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "../include/files.h"
#include "../include/logger.h"

// returns heap allocated buffer containing file contents
// caller is responsible for calling free() on returned pointer
// returns NULL if file not found or cannot be read
char *read_file(const char *path, size_t *file_size) {
    // block path traversal attacks
    // reject any path containing ".." 
    if (strstr(path, "..") != NULL) {
        log_error("path traversal attempt blocked");
        return NULL;
    }
    // 1. build the full path
    char full_path[512]; // big enough for "static/" + path

    // 2. if path is "/" serve "static/index.html" by default
    if (strcmp(path, "/") == 0) {
        snprintf(full_path, sizeof(full_path), "static/index.html");
    } else {
        // otherwise, prepend "static" to the requested path
        snprintf(full_path, sizeof(full_path), "static%s", path);
    }

    // 3. use stat() to get file size - returns NULL if file does not exist
    struct stat file_stat;
    if (stat(full_path, &file_stat) < 0) {
        return NULL;
    }
    // caller needs this to know how many bytes to send
    *file_size = file_stat.st_size;

    // 4. malloc a buffer of file_size + 1 bytes
    char *buffer = malloc(file_stat.st_size + 1);
    if (!buffer) {
        perror("malloc");
        return NULL;
    }

    // 5. fopen the file in binary read mode
    FILE *fp = fopen(full_path, "rb");
    if (!fp) {
        perror("fopen");
        free(buffer);
        return NULL;
    }

    // 6. fread contents into buffer
    size_t bytes_read = fread(buffer, 1, file_stat.st_size, fp);
    if (bytes_read != (size_t)file_stat.st_size) {
        perror("fread");
        fclose(fp);
        free(buffer);
        return NULL;
    }

    // 7. null terminate the buffer
    buffer[file_stat.st_size] = '\0';

    // 8. fclose the file
    fclose(fp);

    // 9. return the buffer
    return buffer;
}