#include <stdlib.h> // malloc, realloc, calloc, free, getenv, setenv, atoi, size_t
#include "client.h"
#include "macros.h" // DEBUG, ERROR
#include "request.h"
#include "response.h"

static client_t *client_init(uv_stream_t *stream);
static void client_on_close(uv_handle_t *handle); // void (*uv_close_cb)(uv_handle_t* handle)
static void client_on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);  // void (*uv_alloc_cb)(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf )
static void client_on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf); // void (*uv_read_cb)(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf )
static int client_error(client_t *client, enum http_status code, char *body, int length);

void client_on_connect(uv_stream_t *stream, int status) { // void (*uv_connection_cb)(uv_stream_t* server, int status)
//    DEBUG("stream=%p, status=%i\n", stream, status);
    if (status) { ERROR("status=%i\n", status); return; }
    client_t *client = client_init(stream);
    if (!client) { ERROR("client_init\n"); return; }
    if (uv_tcp_init(stream->loop, &client->tcp)) { ERROR("uv_tcp_init\n"); client_free(client); return; } // int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle)
    if (uv_accept(stream, (uv_stream_t *)&client->tcp)) { ERROR("uv_accept\n"); client_free(client); return; } // int uv_accept(uv_stream_t* server, uv_stream_t* client)
    struct sockaddr_in sockname, peername; int socklen = sizeof(struct sockaddr), peerlen = sizeof(struct sockaddr);
    if (uv_tcp_getsockname(&client->tcp, (struct sockaddr *)&sockname, &socklen)) { ERROR("uv_tcp_getsockname\n"); client_close(client); return; } // int uv_tcp_getsockname(const uv_tcp_t* handle, struct sockaddr* name, int* namelen)
    if (uv_tcp_getpeername(&client->tcp, (struct sockaddr *)&peername, &peerlen)) { ERROR("uv_tcp_getpeername\n"); client_close(client); return; } // int uv_tcp_getpeername(const uv_tcp_t* handle, struct sockaddr* name, int* namelen)
    client->server_port = ntohs(sockname.sin_port); client->client_port = ntohs(peername.sin_port);
    if (uv_ip4_name(&sockname, client->server_ip, sizeof(client->server_ip))) { ERROR("uv_ip4_name\n"); client_close(client); return; } //int uv_ip4_name(const struct sockaddr_in* src, char* dst, size_t size)
    if (uv_ip4_name(&peername, client->client_ip, sizeof(client->client_ip))) { ERROR("uv_ip4_name\n"); client_close(client); return; } //int uv_ip4_name(const struct sockaddr_in* src, char* dst, size_t size)
    if (uv_read_start((uv_stream_t *)&client->tcp, client_on_alloc, client_on_read)) { ERROR("uv_read_start\n"); client_close(client); return; } // int uv_read_start(uv_stream_t* stream, uv_alloc_cb alloc_cb, uv_read_cb read_cb)
}

static client_t *client_init(uv_stream_t *stream) {
//    DEBUG("stream=%p\n", stream);
    client_t *client = (client_t *)malloc(sizeof(client_t));
    if (!client) { ERROR("malloc\n"); return NULL; }
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
    if (client->tcp.type != UV_TCP) { ERROR("client=%p, client->tcp.type=%i\n", client, client->tcp.type); return; }
    if (client->tcp.flags > MAX_FLAG) { ERROR("client=%p, client->tcp.flags=%u\n", client, client->tcp.flags); /*client_free(client);*/ return; }
    uv_handle_t *handle = (uv_handle_t *)&client->tcp;
    if (handle->type != UV_TCP) { ERROR("handle->type=%i\n", handle->type); return; }
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
    if (!buf->base) { ERROR("malloc\n"); client_close(client); return; }
    buf->len = suggested_size;
}

static void client_on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) { // void (*uv_read_cb)(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf )
//    if (nread >= 0) DEBUG("stream=%p, nread=%li, buf->base=%p\n<<\n%.*s\n>>\n", stream, nread, buf->base, (int)nread, buf->base);
//    DEBUG("nread=%li\n", nread);
    client_t *client = (client_t *)stream->data;
    switch (nread) {
        case 0: return; // no-op - there's no data to be read, but there might be later
        case UV_ENOBUFS: ERROR("UV_ENOBUFS\n"); if (client_error(client, HTTP_STATUS_PAYLOAD_TOO_LARGE, "payload too large", sizeof("payload too large") - 1)) ERROR("client_error\n"); break;
        case UV_ECONNRESET: ERROR("UV_ECONNRESET\n"); client_close(client); break;
        case UV_ECONNABORTED: ERROR("UV_ECONNABORTED\n"); client_close(client); break;
        case UV_EOF: /*ERROR("UV_EOF\n"); */parser_init_or_client_close(client); break;
        default: {
            if (nread < 0) {
                ERROR("nread=%li\n", nread);
                if (client_error(client, HTTP_STATUS_INTERNAL_SERVER_ERROR, "internal server error", sizeof("internal server error") - 1)) ERROR("client_error\n");
            } else {
                if ((ssize_t)parser_execute(client, buf->base, nread) < nread) { if (client_error(client, HTTP_STATUS_BAD_REQUEST, "bad request", sizeof("bad request") - 1)) ERROR("client_error\n"); }
                if (HTTP_PARSER_ERRNO(&client->parser)) {
                    ERROR("parser_execute(%i)%s\n", HTTP_PARSER_ERRNO(&client->parser), http_errno_description(HTTP_PARSER_ERRNO(&client->parser)));
                    if (client_error(client, HTTP_STATUS_BAD_REQUEST, "bad request", sizeof("bad request") - 1)) ERROR("client_error\n");
                }
            }
        } break;
    }
    if (buf->base) free(buf->base);
}

static int client_error(client_t *client, enum http_status code, char *body, int length) {
    int error = 0;
    if ((error = uv_read_stop((uv_stream_t *)&client->tcp))) { ERROR("uv_read_stop\n"); return error; } // int uv_read_stop(uv_stream_t*) // ???
    if ((error = client_response(client, code, body, length))) { ERROR("client_response\n"); return error; }
    client_close(client); // ???
    return error;
}

int client_response(client_t *client, enum http_status code, char *body, int length) {
    int error = 0;
    if ((error = client->tcp.type != UV_TCP)) { ERROR("client=%p, client->tcp.type=%i\n", client, client->tcp.type); return error; }
    if ((error = client->tcp.flags > MAX_FLAG)) { ERROR("client=%p, client->tcp.flags=%u\n", client, client->tcp.flags); return error; }
    if ((error = uv_is_closing((const uv_handle_t *)&client->tcp))) { ERROR("uv_is_closing\n"); return error; } // int uv_is_closing(const uv_handle_t* handle)
    if ((error = response_write(client, code, body, length))) { ERROR("response_write\n"); client_close(client); return error; } // char *PQgetvalue(const PGresult *res, int row_number, int column_number); int PQgetlength(const PGresult *res, int row_number, int column_number)
    return error;
}
