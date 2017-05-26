#include <stdlib.h>
#include <sys/sysctl.h>
#include <linux/sysctl.h>
#include <uv.h>
#include "macros.h"
#include "context.h"
#include "request.h"

void request_on_start(void *arg) { // void (*uv_thread_cb)(void* arg)
    int name[] = {CTL_NET, NET_CORE, NET_CORE_SOMAXCONN}, nlen = sizeof(name), oldval[nlen];
    size_t oldlenp = sizeof(oldval);
    if (sysctl(name, nlen / sizeof(int), (void *)oldval, &oldlenp, NULL, 0)) {// int sysctl (int *name, int nlen, void *oldval, size_t *oldlenp, void *newval, size_t newlen)
        ERROR("sysctl\n");
        return;
    }
    int backlog = SOMAXCONN;
    if (oldlenp > 0) backlog = oldval[0];
    uv_loop_t loop;
    if (uv_loop_init(&loop)) { // int uv_loop_init(uv_loop_t* loop)
        ERROR("uv_loop_init\n");
        return;
    }
    server_t *server = (server_t *)malloc(sizeof(server_t));
    if (server == NULL) {
        ERROR("malloc\n");
        return;
    }
    loop.data = server;
    server->conninfo = getenv("WEB_SERVER_PGCONN"); // char *getenv(const char *name)
    if (!server->conninfo) server->conninfo = "postgresql://localhost";
    if (postgres_connect(&loop)) {
        ERROR("postgres_connect\n");
        free(server);
        return;
    }
    uv_tcp_t handle;
    if (uv_tcp_init(&loop, &handle)) { // int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle)
        ERROR("uv_tcp_init\n");
        free(server);
        return;
    }
    uv_os_sock_t client_sock = *((uv_os_sock_t *)arg);
    if (uv_tcp_open(&handle, client_sock)) { // int uv_tcp_open(uv_tcp_t* handle, uv_os_sock_t sock)
        ERROR("uv_tcp_open\n");
        free(server);
        return;
    }
    if (uv_listen((uv_stream_t *)&handle, backlog, request_on_connect)) { // int uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb)
        free(server);
        ERROR("uv_listen\n");
        return;
    }
    if (uv_run(&loop, UV_RUN_DEFAULT)) { // int uv_run(uv_loop_t* loop, uv_run_mode mode)
        ERROR("uv_run\n");
        free(server);
        return;
    }
}

void request_on_connect(uv_stream_t *server, int status) { // void (*uv_connection_cb)(uv_stream_t* server, int status)
    if (status) {
        ERROR("status");
        return;
    }
    client_t *client = (client_t *)malloc(sizeof(client_t));
    if (client == NULL) {
        ERROR("malloc\n");
        return;
    }
    if (uv_tcp_init(server->loop, &client->tcp)) { // int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle)
        ERROR("uv_tcp_init\n");
        free(client);
        return;
    }
    client->tcp.data = client;
    if (uv_accept(server, (uv_stream_t *)&client->tcp)) { // int uv_accept(uv_stream_t* server, uv_stream_t* client)
        ERROR("uv_accept\n");
        request_close((uv_handle_t *)&client->tcp);
        return;
    }
    if (uv_read_start((uv_stream_t *)&client->tcp, parser_on_alloc, parser_on_read)) { // int uv_read_start(uv_stream_t* stream, uv_alloc_cb alloc_cb, uv_read_cb read_cb)
        ERROR("uv_read_start\n");
        request_close((uv_handle_t *)&client->tcp);
        return;
    }
}

void request_close(uv_handle_t *handle) {
    if (!uv_is_closing(handle)) { // int uv_is_closing(const uv_handle_t* handle)
        uv_close(handle, request_on_close); // void uv_close(uv_handle_t* handle, uv_close_cb close_cb)
    }
}

void request_on_close(uv_handle_t *handle) { // void (*uv_close_cb)(uv_handle_t* handle)
    client_t *client = (client_t *)handle->data;
    free(client);
}
