#ifndef _SERVER_H
#define _SERVER_H

#include "queue.h" // queue_*
#include <uv.h> // uv_*

typedef struct server_t {
    queue_t postgres_queue;
    queue_t client_queue;
    queue_t request_queue;
} server_t;

void server_on_start(void *arg); // void (*uv_thread_cb)(void* arg)

#endif // _SERVER_H
