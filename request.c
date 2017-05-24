#include <stdlib.h>
#include <uv.h>
#include "macros.h"
#include "context.h"
#include "request.h"

void request_on_start(void *arg) { // void (*uv_thread_cb)(void* arg)
    uv_loop_t loop;
    if (uv_loop_init(&loop)) { // int uv_loop_init(uv_loop_t* loop)
        ERROR("uv_loop_init\n");
        return;
    }
    uv_tcp_t handle;
    if (uv_tcp_init(&loop, &handle)) { // int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle)
        ERROR("uv_tcp_init\n");
        return;
    }
    if (uv_tcp_open(&handle, *((uv_os_sock_t *)arg))) { // int uv_tcp_open(uv_tcp_t* handle, uv_os_sock_t sock)
        ERROR("uv_tcp_open\n");
        return;
    }
#   define BACKLOG 128
    if (uv_listen((uv_stream_t *)&handle, BACKLOG, request_on_connect)) { // int uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb)
        ERROR("uv_listen\n");
        return;
    }
    if (uv_run(&loop, UV_RUN_DEFAULT)) { // int uv_run(uv_loop_t* loop, uv_run_mode mode)
        ERROR("uv_run\n");
        return;
    }
}

void request_on_connect(uv_stream_t *server, int status) { // void (*uv_connection_cb)(uv_stream_t* server, int status)
    if (status) {
        ERROR("status");
        return;
    }
    context_t *context = (context_t *)malloc(sizeof(context_t));
    if (context == NULL) {
        ERROR("malloc\n");
        return;
    }
    if (uv_tcp_init(server->loop, &context->client)) { // int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle)
        ERROR("uv_tcp_init\n");
        free(context);
        return;
    }
    context->client.data = context;
    if (uv_accept(server, (uv_stream_t *)&context->client)) { // int uv_accept(uv_stream_t* server, uv_stream_t* client)
        ERROR("uv_accept\n");
        request_close((uv_handle_t *)&context->client);
        return;
    }
    if (uv_read_start((uv_stream_t *)&context->client, parser_on_alloc, parser_on_read)) { // int uv_read_start(uv_stream_t* stream, uv_alloc_cb alloc_cb, uv_read_cb read_cb)
        ERROR("uv_read_start\n");
        request_close((uv_handle_t *)&context->client);
        return;
    }
}

void request_close(uv_handle_t * handle) {
    if (!uv_is_closing(handle)) { // int uv_is_closing(const uv_handle_t* handle)
        uv_close(handle, request_on_close); // void uv_close(uv_handle_t* handle, uv_close_cb close_cb)
    }
}

void request_on_close(uv_handle_t *handle) { // void (*uv_close_cb)(uv_handle_t* handle)
    context_t *context = (context_t *)handle->data;
    free(context);
}
