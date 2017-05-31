#include <stdlib.h> // malloc, free, size_t
#include <sys/sysctl.h> // sysctl, CTL_NET, NET_CORE, NET_CORE_SOMAXCONN
#include "request.h"
#include "macros.h"

void request_on_start(void *arg) { // void (*uv_thread_cb)(void* arg)
    uv_loop_t loop;
    if (uv_loop_init(&loop)) { ERROR("uv_loop_init\n"); return; } // int uv_loop_init(uv_loop_t* loop)
    server_t *server = (server_t *)malloc(sizeof(server_t));
    if (!server) { ERROR("malloc\n"); return; }
    loop.data = (void *)server;
    if (postgres_queue(&loop)) { ERROR("postgres_queue\n"); free(server); return; }
    uv_tcp_t tcp;
    if (uv_tcp_init(&loop, &tcp)) { ERROR("uv_tcp_init\n"); free(server); return; } // int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle)
    uv_os_sock_t client_sock = *((uv_os_sock_t *)arg);
    if (uv_tcp_open(&tcp, client_sock)) { ERROR("uv_tcp_open\n"); free(server); return; } // int uv_tcp_open(uv_tcp_t* handle, uv_os_sock_t sock)
    int name[] = {CTL_NET, NET_CORE, NET_CORE_SOMAXCONN}, nlen = sizeof(name), oldval[nlen]; size_t oldlenp = sizeof(oldval);
    if (sysctl(name, nlen / sizeof(int), (void *)oldval, &oldlenp, NULL, 0)) { ERROR("sysctl\n"); free(server); return; } // int sysctl (int *name, int nlen, void *oldval, size_t *oldlenp, void *newval, size_t newlen)
    int backlog = SOMAXCONN;
    if (oldlenp > 0) backlog = oldval[0];
    if (uv_listen((uv_stream_t *)&tcp, backlog, request_on_connect)) { ERROR("uv_listen\n"); free(server); return; } // int uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb)
    if (uv_run(&loop, UV_RUN_DEFAULT)) { ERROR("uv_run\n"); free(server); return; } // int uv_run(uv_loop_t* loop, uv_run_mode mode)
}

void request_on_connect(uv_stream_t *server, int status) { // void (*uv_connection_cb)(uv_stream_t* server, int status)
    if (status) { ERROR("status=%i\n", status); return; }
    client_t *client = (client_t *)malloc(sizeof(client_t));
    if (!client) { ERROR("malloc\n"); return; }
    queue_init(&client->request_queue);
    if (uv_tcp_init(server->loop, &client->tcp)) { ERROR("uv_tcp_init\n"); free(client); return; } // int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle)
    client->tcp.data = client;
    if (uv_accept(server, (uv_stream_t *)&client->tcp)) { ERROR("uv_accept\n"); request_close(client); return; } // int uv_accept(uv_stream_t* server, uv_stream_t* client)
    if (uv_read_start((uv_stream_t *)&client->tcp, parser_on_alloc, parser_on_read)) { ERROR("uv_read_start\n"); request_close(client); return; } // int uv_read_start(uv_stream_t* stream, uv_alloc_cb alloc_cb, uv_read_cb read_cb)
}

void request_close(client_t *client) {
    uv_handle_t *handle = (uv_handle_t *)&client->tcp;
/*    while (!queue_empty(&client->request_queue)) {
        queue_t *queue = queue_head(&client->request_queue); queue_remove(queue); queue_init(queue);
        request_t *request = queue_data(queue, request_t, client_queue);
        DEBUG("request=%p, request->postgres=%p\n", request, request->postgres);
        request_free(request);
    }*/
//    if (handle->type != UV_TCP) { ERROR("handle->type=%i\n", handle->type); /*free(client); */return; }
    if (!uv_is_closing(handle)) uv_close(handle, request_on_close); // int uv_is_closing(const uv_handle_t* handle); void uv_close(uv_handle_t* handle, uv_close_cb close_cb)
}

void request_on_close(uv_handle_t *handle) { // void (*uv_close_cb)(uv_handle_t* handle)
    DEBUG("handle=%p\n", handle);
    client_t *client = (client_t *)handle->data;
    while (!queue_empty(&client->request_queue)) {
        queue_t *queue = queue_head(&client->request_queue); queue_remove(queue); queue_init(queue);
        request_t *request = queue_data(queue, request_t, client_queue);
        DEBUG("request=%p, request->postgres=%p\n", request, request->postgres);
        request_free(request);
    }
    free(client);
}

void request_free(request_t *request) {
    DEBUG("request=%p, request->postgres=%p\n", request, request->postgres);
    if (request->postgres) postgres_push_postgres(request->postgres);
    queue_remove(&request->client_queue);
    free(request);
}
