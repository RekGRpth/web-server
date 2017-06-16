#ifndef _SERVER_H
#define _SERVER_H

typedef struct server_s server_t;

#include "queue.h" // queue_*
#include <uv.h> // uv_*

struct server_s {
    queue_t postgres_queue;
    queue_t client_queue;
    queue_t request_queue;
};

void server_on_start(void *arg); // void (*uv_thread_cb)(void* arg)

#endif // _SERVER_H
