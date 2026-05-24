#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../include/threadpool.h"

static void *worker(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;

    while (1) {
        // STEP 1: grab the lock
        // nobody else can touch queue while we hold this
        pthread_mutex_lock(&pool->lock);

        // STEP 2: wait if nothing to do
        // pthread_cond_wait atomically:
        //   releases the lock (so others can add work)
        //   puts this thread to sleep
        //   when signaled: re-acquires lock and returns
        while (pool->count == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->has_work, &pool->lock);
        }

        // STEP 3: check if we should exit
        if (pool->shutdown && pool->count == 0) {
            pthread_mutex_unlock(&pool->lock);
            return NULL;
        }

        // STEP 4: take fd from queue
        int fd = pool->queue[pool->head];

        // STEP 5: advance head (circular buffer)
        // % QUEUE_SIZE wraps around:
        // head 127 + 1 = 128 % 128 = 0 (back to start)
        pool->head = (pool->head + 1) % QUEUE_SIZE;
        pool->count--;

        // STEP 6: tell main thread queue has space
        pthread_cond_signal(&pool->has_space);

        // STEP 7: release lock BEFORE handling request
        // critical — don't hold lock while doing slow work
        // other workers can run while we handle this request
        pthread_mutex_unlock(&pool->lock);

        // STEP 8: handle the request
        // this is the slow part — reading, parsing, sending
        // other workers are handling other fds simultaneously
        pool->handler(fd);
    }

    return NULL;
}

ThreadPool *threadpool_create(void (*handler)(int fd)) {
    // 1. malloc the pool
    ThreadPool *pool = malloc(sizeof(ThreadPool));
    if (!pool) return NULL;
    // 2. initialize head, tail, count, shutdown to 0
    pool->head = 0;
    pool->tail = 0;
    pool->count = 0;
    pool->shutdown = 0;
    // 3. set pool->handler
    pool->handler = handler;
    // 4. initialize mutex and condition variables
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->has_work, NULL);
    pthread_cond_init(&pool->has_space, NULL);
    // 5. create THREAD_COUNT threads
    // each thread runs worker() with pool as argument
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_create(&pool->threads[i], NULL, worker, pool);
    }
    // 6. return pool
    return pool;
}

void threadpool_add(ThreadPool *pool, int fd) {
    // 1. lock mutex
    pthread_mutex_lock(&pool->lock);
    // 2. wait while queue is full
    while (pool->count == QUEUE_SIZE) {
        pthread_cond_wait(&pool->has_space, &pool->lock);
    }
    // 3. add fd to queue[tail]
    pool->queue[pool->tail] = fd;
    // 4. update tail and count
    pool->tail = (pool->tail + 1) % QUEUE_SIZE;
    pool->count++;
    // 5. signal has_work
    pthread_cond_signal(&pool->has_work);
    // 6. unlock mutex
    pthread_mutex_unlock(&pool->lock);
}

void threadpool_destroy(ThreadPool *pool) {
    // your code here
    // 1. lock mutex
    pthread_mutex_lock(&pool->lock);
    // 2. set shutdown = 1
    pool->shutdown = 1;
    // 3. broadcast has_work
    pthread_cond_broadcast(&pool->has_work);
    // 4. unlock mutex
    pthread_mutex_unlock(&pool->lock);
    // 5. join all threads
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    // 6. destroy mutex and condition variables
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->has_work);
    pthread_cond_destroy(&pool->has_space);
    // 7. free pool
    free(pool);
}