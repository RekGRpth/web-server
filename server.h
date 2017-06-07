#ifndef _SERVER_H
#define _SERVER_H

#include <uv.h> // uv_*
#include "queue.h" // queue_*
//#include "context.h"

typedef struct server_t {
    queue_t postgres_queue;
    queue_t client_queue;
    queue_t request_queue;
} server_t;

void server_on_start(void *arg); // void (*uv_thread_cb)(void* arg)
server_t *server_init(uv_loop_t *loop);
void server_free(server_t *server);
void server_postgres(uv_loop_t *loop);

#endif // _SERVER_H
