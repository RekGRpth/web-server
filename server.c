#include <stdlib.h> // malloc, realloc, calloc, free, getenv, setenv, atoi, size_t
#include <sys/sysctl.h> // sysctl, CTL_NET, NET_CORE, NET_CORE_SOMAXCONN
#include "server.h"
#include "macros.h" // DEBUG, ERROR
#include "postgres.h"

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
    server_postgres(loop);
    return server;
}

void server_free(server_t *server) {
    ERROR("server=%p\n", server);
    while (!queue_empty(&server->postgres_queue)) postgres_free(pointer_data(queue_get_pointer(&server->postgres_queue), postgres_t, server_pointer));
    while (!queue_empty(&server->client_queue)) client_free(pointer_data(queue_get_pointer(&server->client_queue), client_t, server_pointer));
    while (!queue_empty(&server->request_queue)) request_free(pointer_data(queue_get_pointer(&server->request_queue), request_t, server_pointer));
    free(server);
}

void server_postgres(uv_loop_t *loop) {
//    DEBUG("loop=%p\n", loop);
    char *conninfo = getenv("WEBSERVER_POSTGRES_CONNINFO"); // char *getenv(const char *name)
    if (!conninfo) conninfo = "postgresql://localhost?application_name=webserver";
    char *webserver_postgres_count = getenv("WEBSERVER_POSTGRES_COUNT"); // char *getenv(const char *name);
    int count = 1;
    if (webserver_postgres_count) count = atoi(webserver_postgres_count);
    if (count < 1) count = 1;
    for (int i = 0; i < count; i++) if (!postgres_init_and_connect(loop, conninfo)) ERROR("postgres_init_and_connect\n");
}
