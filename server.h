#ifndef _SERVER_H
#define _SERVER_H

#include "context.h"

void server_on_start(void *arg); // void (*uv_thread_cb)(void* arg)
server_t *server_init(uv_loop_t *loop);
void server_free(server_t *server);

#endif // _SERVER_H
