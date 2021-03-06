#include "client.h" // client_*
#include "request.h" // request_t, request_free
#include "response.h" // response_code_body
#include "macros.h" // DEBUG, ERROR, FATAL
#include <stdlib.h> // malloc, realloc, calloc, free, getenv, setenv, atoi, size_t

static client_t *client_init(uv_stream_t *stream);
static void client_on_close(uv_handle_t *handle); // void (*uv_close_cb)(uv_handle_t* handle)
static void client_on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);  // void (*uv_alloc_cb)(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf )
static void client_on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf); // void (*uv_read_cb)(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf )
static int client_error(client_t *client, enum http_status code, char *body, int length);

void client_on_connect(uv_stream_t *stream, int status) { // void (*uv_connection_cb)(uv_stream_t* server, int status)
//    DEBUG("stream=%p, status=%i\n", stream, status);
    if (status) { FATAL("status=%i\n", status); return; }
    client_t *client = client_init(stream);
    if (!client) { FATAL("client_init\n"); return; }
    if (uv_tcp_init(stream->loop, &client->tcp)) { FATAL("uv_tcp_init\n"); client_free(client); return; } // int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle)
    if (uv_accept(stream, (uv_stream_t *)&client->tcp)) { FATAL("uv_accept\n"); client_free(client); return; } // int uv_accept(uv_stream_t* server, uv_stream_t* client)
    struct sockaddr_in sockname, peername; int socklen = sizeof(struct sockaddr), peerlen = sizeof(struct sockaddr);
    if (uv_tcp_getsockname(&client->tcp, (struct sockaddr *)&sockname, &socklen)) { FATAL("uv_tcp_getsockname\n"); client_close(client); return; } // int uv_tcp_getsockname(const uv_tcp_t* handle, struct sockaddr* name, int* namelen)
    if (uv_tcp_getpeername(&client->tcp, (struct sockaddr *)&peername, &peerlen)) { FATAL("uv_tcp_getpeername\n"); client_close(client); return; } // int uv_tcp_getpeername(const uv_tcp_t* handle, struct sockaddr* name, int* namelen)
    client->server_port = ntohs(sockname.sin_port); client->client_port = ntohs(peername.sin_port);
    if (uv_ip4_name(&sockname, client->server_ip, sizeof(client->server_ip))) { FATAL("uv_ip4_name\n"); client_close(client); return; } //int uv_ip4_name(const struct sockaddr_in* src, char* dst, size_t size)
    if (uv_ip4_name(&peername, client->client_ip, sizeof(client->client_ip))) { FATAL("uv_ip4_name\n"); client_close(client); return; } //int uv_ip4_name(const struct sockaddr_in* src, char* dst, size_t size)
    if (uv_read_start((uv_stream_t *)&client->tcp, client_on_alloc, client_on_read)) { FATAL("uv_read_start\n"); client_close(client); return; } // int uv_read_start(uv_stream_t* stream, uv_alloc_cb alloc_cb, uv_read_cb read_cb)
}

static client_t *client_init(uv_stream_t *stream) {
//    DEBUG("stream=%p\n", stream);
    client_t *client = (client_t *)malloc(sizeof(client_t));
    if (!client) { FATAL("malloc\n"); return NULL; }
    queue_init(&client->request_queue);
    pointer_init(&client->server_pointer);
    client->tcp.data = (void *)client;
    server_t *server = (server_t *)stream->loop->data;
    queue_put_pointer(&server->client_queue, &client->server_pointer);
    parser_init(client);
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
    if (client->tcp.type != UV_TCP) { FATAL("client=%p, client->tcp.type=%i\n", client, client->tcp.type); return; }
    if (client->tcp.flags > MAX_FLAG) { FATAL("client=%p, client->tcp.flags=%u\n", client, client->tcp.flags); /*client_free(client);*/ return; }
    uv_handle_t *handle = (uv_handle_t *)&client->tcp;
    if (handle->type != UV_TCP) { FATAL("handle->type=%i\n", handle->type); return; }
    if (!uv_is_closing(handle)) uv_close(handle, client_on_close); // int uv_is_closing(const uv_handle_t* handle); void uv_close(uv_handle_t* handle, uv_close_cb close_cb)
}

static void client_on_close(uv_handle_t *handle) { // void (*uv_close_cb)(uv_handle_t* handle)
//    DEBUG("handle=%p\n", handle);
    client_t *client = (client_t *)handle->data;
    client_free(client);
}

static void client_on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) { // void (*uv_alloc_cb)(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf )
//    DEBUG("handle=%p, suggested_size=%li, buf=%p\n", handle, suggested_size, buf);
//    suggested_size = 8;
//    suggested_size = 16;
//    suggested_size = 128;
    buf->base = (char *)malloc(suggested_size);
    client_t *client = (client_t *)handle->data;
    if (!buf->base) { FATAL("malloc\n"); client_close(client); return; }
    buf->len = suggested_size;
}

static void client_on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) { // void (*uv_read_cb)(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf )
//    if (nread >= 0) DEBUG("stream=%p, nread=%li, buf->base=%p\n<<\n%.*s\n>>\n", stream, nread, buf->base, (int)nread, buf->base);
//    DEBUG("nread=%li\n", nread);
    client_t *client = (client_t *)stream->data;
    if (nread > 0) {
        ssize_t parsed = (ssize_t)parser_execute(client, buf->base, nread);
        if (parsed < nread) {
            ERROR("parsed=%li, nread=%li\n", parsed, nread);
            if (client_error(client, HTTP_STATUS_BAD_REQUEST, "bad request", sizeof("bad request") - 1)) FATAL("client_error\n");
        }
        if (HTTP_PARSER_ERRNO(&client->parser)) {
            FATAL("parser_execute(%i)%s\n", HTTP_PARSER_ERRNO(&client->parser), http_errno_description(HTTP_PARSER_ERRNO(&client->parser)));
            if (client_error(client, HTTP_STATUS_BAD_REQUEST, "bad request", sizeof("bad request") - 1)) FATAL("client_error\n");
        }
    } else switch (nread) {
        case 0: break; // no-op - there's no data to be read, but there might be later
        case UV_EOF: /*FATAL("UV_EOF\n"); */parser_init_or_client_close(client); break;
        case UV_ENOBUFS: FATAL("UV_ENOBUFS\n"); if (client_error(client, HTTP_STATUS_PAYLOAD_TOO_LARGE, "payload too large", sizeof("payload too large") - 1)) FATAL("client_error\n"); break;
        case UV_ECONNRESET: FATAL("UV_ECONNRESET\n"); client_close(client); break;
        case UV_ECONNABORTED: FATAL("UV_ECONNABORTED\n"); client_close(client); break;
        default: FATAL("nread=%li\n", nread); if (client_error(client, HTTP_STATUS_INTERNAL_SERVER_ERROR, "internal server error", sizeof("internal server error") - 1)) FATAL("client_error\n"); break;
    }
    if (buf->base) free(buf->base);
}

static int client_error(client_t *client, enum http_status code, char *body, int length) {
    int error = 0;
    if ((error = uv_read_stop((uv_stream_t *)&client->tcp))) { FATAL("uv_read_stop\n"); return error; } // int uv_read_stop(uv_stream_t*) // ???
    if ((error = response_code_body(client, code, body, length))) { FATAL("response_code_body\n"); return error; }
    client_close(client); // ???
    return error;
}
