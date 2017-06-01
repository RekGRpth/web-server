#include <stdlib.h> // malloc, free, size_t
#include <sys/sysctl.h> // sysctl, CTL_NET, NET_CORE, NET_CORE_SOMAXCONN
#include "request.h"
#include "macros.h"

void request_on_start(void *arg) { // void (*uv_thread_cb)(void* arg)
    uv_loop_t loop;
    if (uv_loop_init(&loop)) { ERROR("uv_loop_init\n"); return; } // int uv_loop_init(uv_loop_t* loop)
    server_t *server = request_server_init(&loop);
    if (!server) { ERROR("request_server_init\n"); return; }
    loop.data = (void *)server;
    postgres_queue(&loop);
    uv_tcp_t tcp;
    if (uv_tcp_init(&loop, &tcp)) { ERROR("uv_tcp_init\n"); request_server_free(server); return; } // int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle)
    uv_os_sock_t client_sock = *((uv_os_sock_t *)arg);
    if (uv_tcp_open(&tcp, client_sock)) { ERROR("uv_tcp_open\n"); request_server_free(server); return; } // int uv_tcp_open(uv_tcp_t* handle, uv_os_sock_t sock)
    int name[] = {CTL_NET, NET_CORE, NET_CORE_SOMAXCONN}, nlen = sizeof(name), oldval[nlen]; size_t oldlenp = sizeof(oldval);
    if (sysctl(name, nlen / sizeof(int), (void *)oldval, &oldlenp, NULL, 0)) { ERROR("sysctl\n"); request_server_free(server); return; } // int sysctl (int *name, int nlen, void *oldval, size_t *oldlenp, void *newval, size_t newlen)
    int backlog = SOMAXCONN;
    if (oldlenp > 0) backlog = oldval[0];
    if (uv_listen((uv_stream_t *)&tcp, backlog, request_on_connect)) { ERROR("uv_listen\n"); request_server_free(server); return; } // int uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb)
    if (uv_run(&loop, UV_RUN_DEFAULT)) { ERROR("uv_run\n"); request_server_free(server); return; } // int uv_run(uv_loop_t* loop, uv_run_mode mode)
}

server_t *request_server_init(uv_loop_t *loop) {
    server_t *server = (server_t *)malloc(sizeof(server_t));
    if (!server) { ERROR("malloc\n"); return NULL; }
    queue_init(&server->postgres_queue);
    queue_init(&server->request_queue);
    queue_init(&server->client_queue);
    return server;
}

void request_server_free(server_t *server) {
    ERROR("server=%p\n", server);
    while (!queue_empty(&server->postgres_queue)) postgres_free(pointer_data(queue_head(&server->postgres_queue), postgres_t, server_pointer));
//    while (!queue_empty(&server->request_queue)) request_free(pointer_data(queue_head(&server->request_queue), request_t, server_pointer));
    while (!queue_empty(&server->client_queue)) request_client_free(pointer_data(queue_head(&server->client_queue), client_t, server_pointer));
    free(server);
}

void request_on_connect(uv_stream_t *server, int status) { // void (*uv_connection_cb)(uv_stream_t* server, int status)
//    DEBUG("server=%p, status=%i\n", server, status);
    if (status) { ERROR("status=%i\n", status); return; }
    client_t *client = request_client_init(server);
    if (!client) { ERROR("request_client_init\n"); return; }
    if (uv_accept(server, (uv_stream_t *)&client->tcp)) { ERROR("uv_accept\n"); request_close(client); return; } // int uv_accept(uv_stream_t* server, uv_stream_t* client)
    if (uv_read_start((uv_stream_t *)&client->tcp, parser_on_alloc, parser_on_read)) { ERROR("uv_read_start\n"); request_close(client); return; } // int uv_read_start(uv_stream_t* stream, uv_alloc_cb alloc_cb, uv_read_cb read_cb)
}

client_t *request_client_init(uv_stream_t *server) {
//    DEBUG("server=%p\n", server);
    client_t *client = (client_t *)malloc(sizeof(client_t));
    if (!client) { ERROR("malloc\n"); return NULL; }
    queue_init(&client->request_queue);
    pointer_init(&client->server_pointer);
    if (uv_tcp_init(server->loop, &client->tcp)) { ERROR("uv_tcp_init\n"); request_client_free(client); return NULL; } // int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle)
    client->tcp.data = (void *)client;
    server_t *server_ = (server_t *)client->tcp.loop->data;
    queue_insert_pointer(&server_->client_queue, &client->server_pointer);
    return client;
}

void request_client_free(client_t *client) {
//    DEBUG("client=%p\n", client);
//    server_t *server = (server_t *)client->tcp.loop->data; DEBUG("queue_count(&server->postgres_queue)=%i, queue_count(&server->client_queue)=%i, queue_count(&server->request_queue)=%i\n", queue_count(&server->postgres_queue), queue_count(&server->client_queue), queue_count(&server->request_queue));
    while (!queue_empty(&client->request_queue)) request_free(pointer_data(queue_head(&client->request_queue), request_t, client_pointer));
    pointer_remove(&client->server_pointer);
    free(client);
}

void request_close(client_t *client) {
//    DEBUG("client=%p\n", client);
    uv_handle_t *handle = (uv_handle_t *)&client->tcp;
    if (client->tcp.type != UV_TCP) { ERROR("client->tcp.type=%i\n", client->tcp.type); return; }
    if (!uv_is_closing(handle)) uv_close(handle, request_on_close); // int uv_is_closing(const uv_handle_t* handle); void uv_close(uv_handle_t* handle, uv_close_cb close_cb)
}

void request_on_close(uv_handle_t *handle) { // void (*uv_close_cb)(uv_handle_t* handle)
//    DEBUG("handle=%p\n", handle);
    client_t *client = (client_t *)handle->data;
    request_client_free(client);
}

request_t *request_init(client_t *client) {
//    DEBUG("client=%p\n", client);
    if (client->tcp.type != UV_TCP) { ERROR("client->tcp.type=%i\n", client->tcp.type); return NULL; }
    if (uv_is_closing((const uv_handle_t *)&client->tcp)) { ERROR("uv_is_closing\n"); return NULL; } // int uv_is_closing(const uv_handle_t* handle)
    request_t *request = (request_t *)malloc(sizeof(request_t));
    if (!request) { ERROR("malloc\n"); return NULL; }
    request->client = client;
    pointer_init(&request->server_pointer);
    pointer_init(&request->client_pointer);
    return request;
}

void request_free(request_t *request) {
//    DEBUG("request=%p, request->postgres=%p\n", request, request->postgres);
//    client_t *client = request->client; server_t *server = (server_t *)client->tcp.loop->data; DEBUG("queue_count(&server->postgres_queue)=%i, queue_count(&server->client_queue)=%i, queue_count(&server->request_queue)=%i\n", queue_count(&server->postgres_queue), queue_count(&server->client_queue), queue_count(&server->request_queue));
    pointer_remove(&request->server_pointer);
    pointer_remove(&request->client_pointer);
    if (request->postgres) if (postgres_push_postgres(request->postgres)) ERROR("postgres_push_postgres\n");
    free(request);
}
