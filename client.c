#include <stdlib.h> // malloc, realloc, calloc, free, getenv, setenv, atoi, size_t
#include "client.h"
#include "macros.h" // DEBUG, ERROR
#include "request.h"

void client_on_connect(uv_stream_t *server, int status) { // void (*uv_connection_cb)(uv_stream_t* server, int status)
//    DEBUG("server=%p, status=%i\n", server, status);
    if (status) { ERROR("status=%i\n", status); return; }
    client_t *client = client_init(server);
    if (!client) { ERROR("client_init\n"); return; }
    if (uv_accept(server, (uv_stream_t *)&client->tcp)) { ERROR("uv_accept\n"); client_free(client); return; } // int uv_accept(uv_stream_t* server, uv_stream_t* client)
    parser_init(client);
    if (uv_read_start((uv_stream_t *)&client->tcp, client_on_alloc, client_on_read)) { ERROR("uv_read_start\n"); client_close(client); return; } // int uv_read_start(uv_stream_t* stream, uv_alloc_cb alloc_cb, uv_read_cb read_cb)
}

client_t *client_init(uv_stream_t *server) {
//    DEBUG("server=%p\n", server);
    client_t *client = (client_t *)malloc(sizeof(client_t));
    if (!client) { ERROR("malloc\n"); return NULL; }
    queue_init(&client->request_queue);
    pointer_init(&client->server_pointer);
    if (uv_tcp_init(server->loop, &client->tcp)) { ERROR("uv_tcp_init\n"); client_free(client); return NULL; } // int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle)
    if (client->tcp.type != UV_TCP) { ERROR("client=%p, client->tcp.type=%i\n", client, client->tcp.type); client_free(client); return NULL; }
    if (client->tcp.flags > MAX_FLAG) { ERROR("client=%p, client->tcp.flags=%u\n", client, client->tcp.flags); client_free(client); return NULL; }
    client->tcp.data = (void *)client;
    server_t *server_ = (server_t *)client->tcp.loop->data;
    queue_put_pointer(&server_->client_queue, &client->server_pointer);
    return client;
}

void client_free(client_t *client) {
//    DEBUG("client=%p\n", client);
//    DEBUG("client=%p, queue_count(&client->request_queue)=%i\n", client, queue_count(&client->request_queue));
//    server_t *server = (server_t *)client->tcp.loop->data; DEBUG("queue_count(&server->postgres_queue)=%i, queue_count(&server->client_queue)=%i, queue_count(&server->request_queue)=%i\n", queue_count(&server->postgres_queue), queue_count(&server->client_queue), queue_count(&server->request_queue));
    while (!queue_empty(&client->request_queue)) request_free(pointer_data(queue_get_pointer(&client->request_queue), request_t, client_pointer));
    pointer_remove(&client->server_pointer);
    free(client);
}

void client_close(client_t *client) {
//    DEBUG("client=%p\n", client);
//    DEBUG("client=%p, client->tcp.flags=%u\n", client, client->tcp.flags);
    if (client->tcp.type != UV_TCP) { ERROR("client=%p, client->tcp.type=%i\n", client, client->tcp.type); return; }
    if (client->tcp.flags > MAX_FLAG) { ERROR("client=%p, client->tcp.flags=%u\n", client, client->tcp.flags); /*client_free(client);*/ return; }
    uv_handle_t *handle = (uv_handle_t *)&client->tcp;
    if (handle->type != UV_TCP) { ERROR("handle->type=%i\n", handle->type); return; }
    if (!uv_is_closing(handle)) uv_close(handle, client_on_close); // int uv_is_closing(const uv_handle_t* handle); void uv_close(uv_handle_t* handle, uv_close_cb close_cb)
}

void client_on_close(uv_handle_t *handle) { // void (*uv_close_cb)(uv_handle_t* handle)
//    DEBUG("handle=%p\n", handle);
    client_t *client = (client_t *)handle->data;
    client_free(client);
}

void client_on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) { // void (*uv_alloc_cb)(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf )
//    DEBUG("handle=%p, suggested_size=%li, buf=%p\n", handle, suggested_size, buf);
//    suggested_size = 8;
//    suggested_size = 16;
//    suggested_size = 128;
    buf->base = (char *)malloc(suggested_size);
    client_t *client = (client_t *)handle->data;
    if (!buf->base) { ERROR("malloc\n"); client_close(client); return; }
//    parser_init(client);
    buf->len = suggested_size;
}

void client_on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) { // void (*uv_read_cb)(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf )
//    if (nread >= 0) DEBUG("stream=%p, nread=%li, buf->base=%p\n<<\n%.*s\n>>\n", stream, nread, buf->base, (int)nread, buf->base);
//    DEBUG("nread=%li\n", nread);
    client_t *client = (client_t *)stream->data;
    if (nread == UV_EOF) { /*ERROR("client=%p, nread=UV_EOF(%li)\n", client, nread); */parser_init_or_client_close(client); }
    else if (nread < 0) { /*ERROR("client=%p, nread=%li\n", client, nread); */client_close(client); }
    else if ((ssize_t)parser_execute(client, buf->base, nread) < nread) { ERROR("parser_execute\n"); client_close(client); }
    if (buf->base) free(buf->base);
}
