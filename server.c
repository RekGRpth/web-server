#include <sys/sysctl.h> // sysctl, CTL_NET, NET_CORE, NET_CORE_SOMAXCONN
#include "server.h"
#include "client.h"
#include "postgres.h"
#include "request.h"

void server_on_start(void *arg) { // void (*uv_thread_cb)(void* arg)
    uv_loop_t loop;
    if (uv_loop_init(&loop)) { ERROR("uv_loop_init\n"); return; } // int uv_loop_init(uv_loop_t* loop)
    server_t *server = server_init(&loop);
    if (!server) { ERROR("server_init\n"); return; }
    uv_tcp_t tcp;
    if (uv_tcp_init(&loop, &tcp)) { ERROR("uv_tcp_init\n"); server_free(server); return; } // int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle)
    uv_os_sock_t client_sock = *((uv_os_sock_t *)arg);
    if (uv_tcp_open(&tcp, client_sock)) { ERROR("uv_tcp_open\n"); server_free(server); return; } // int uv_tcp_open(uv_tcp_t* handle, uv_os_sock_t sock)
    int name[] = {CTL_NET, NET_CORE, NET_CORE_SOMAXCONN}, nlen = sizeof(name), oldval[nlen]; size_t oldlenp = sizeof(oldval);
    if (sysctl(name, nlen / sizeof(int), (void *)oldval, &oldlenp, NULL, 0)) { ERROR("sysctl\n"); server_free(server); return; } // int sysctl (int *name, int nlen, void *oldval, size_t *oldlenp, void *newval, size_t newlen)
    int backlog = SOMAXCONN; if (oldlenp > 0) backlog = oldval[0];
    if (uv_listen((uv_stream_t *)&tcp, backlog, client_on_connect)) { ERROR("uv_listen\n"); server_free(server); return; } // int uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb)
    if (uv_run(&loop, UV_RUN_DEFAULT)) { ERROR("uv_run\n"); server_free(server); return; } // int uv_run(uv_loop_t* loop, uv_run_mode mode)
}

server_t *server_init(uv_loop_t *loop) {
    server_t *server = (server_t *)malloc(sizeof(server_t));
    if (!server) { ERROR("malloc\n"); return NULL; }
    queue_init(&server->postgres_queue);
    queue_init(&server->request_queue);
    queue_init(&server->client_queue);
    loop->data = (void *)server;
    postgres_queue(loop);
    return server;
}

void server_free(server_t *server) {
    ERROR("server=%p\n", server);
    while (!queue_empty(&server->postgres_queue)) postgres_free(pointer_data(queue_head(&server->postgres_queue), postgres_t, server_pointer));
    while (!queue_empty(&server->client_queue)) client_free(pointer_data(queue_head(&server->client_queue), client_t, server_pointer));
    while (!queue_empty(&server->request_queue)) request_free(pointer_data(queue_head(&server->request_queue), request_t, server_pointer));
    free(server);
}
