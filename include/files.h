#ifndef FILES_H
#define FILES_H

#include <stddef.h> // size_t

// returns heap allocated buffer containing file contents
// caller must call free() on returned pointer
// returns NULL if file not found or cannot be read
char *read_file(const char *path, size_t *file_size);

#endif