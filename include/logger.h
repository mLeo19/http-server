#ifndef LOGGER_H
#define LOGGER_H

// log a completed HTTP request
// called after every request is handled
void log_request(const char *method,
                 const char *path,
                 int status,
                 double ms);

// log a server event (startup, shutdown, errors)
void log_info(const char *message);
void log_error(const char *message);

#endif