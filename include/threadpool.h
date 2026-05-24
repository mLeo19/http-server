#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

#define THREAD_COUNT 8
#define QUEUE_SIZE   128

typedef struct {
    int queue[QUEUE_SIZE];    // file descriptors waiting
    int head;                 // next fd to take
    int tail;                 // where to add next fd
    int count;                // how many fds in queue

    pthread_t threads[THREAD_COUNT]; // worker threads
    pthread_mutex_t lock;            // protects queue
    pthread_cond_t  has_work;        // signals workers
    pthread_cond_t  has_space;       // signals when queue not full

    int shutdown;             // set to 1 to stop workers
    void (*handler)(int fd);  // function to call per fd
} ThreadPool;

// create and start thread pool
ThreadPool *threadpool_create(void (*handler)(int fd));

// add a file descriptor to the queue
// blocks if queue is full
void threadpool_add(ThreadPool *pool, int fd);

// shutdown and free the thread pool
void threadpool_destroy(ThreadPool *pool);

#endif