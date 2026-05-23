#include <stdio.h>
#include <time.h>
#include "../include/logger.h"

// helper — fills buf with current timestamp string
static void get_timestamp(char *buf, size_t size) {
    time_t now = time(NULL);           // current time as integer
    struct tm *t = localtime(&now);    // convert to local time struct
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", t); // format as string
}

void log_request(const char *method,
                 const char *path,
                 int status,
                 double ms) {
    // 1. declare a char buf[32] for the timestamp
    char buf[32];
    // 2. call get_timestamp(buf, sizeof buf)
    get_timestamp(buf, sizeof buf);
    // 3. printf the log line using the format: "[timestamp] method path status ms"
    printf("[%s] %s %s %d %.2f ms\n", buf, method, path, status, ms);
    fflush(stdout); // flush immediately so logs appear in real time even if the program crashes
}

void log_info(const char *message) {
    // get timestamp, printf with [INFO] label
    char buf[32];
    get_timestamp(buf, sizeof buf);
    printf("[%s] [INFO] %s\n", buf, message);
    fflush(stdout);
}

void log_error(const char *message) {
    // get timestamp, printf with [ERROR] label
    char buf[32];
    get_timestamp(buf, sizeof buf);
    fprintf(stderr, "[%s] [ERROR] %s\n", buf, message);
    fflush(stderr);
}