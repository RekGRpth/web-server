#include <stdlib.h> // malloc, realloc, calloc, free, getenv, setenv, atoi, size_t
#include "response.h"
#include "macros.h" // DEBUG, ERROR

#define CRLF "\r\n"

#define HEADERS \
    "Content-Type: application/json; charset=utf-8" CRLF \
    "Content-Length: %d" CRLF \
    "Connection: keep-alive" CRLF

#define STATUS "HTTP/%d.%d %s" CRLF

static void response_on_write(uv_write_t *req, int status); // void (*uv_write_cb)(uv_write_t* req, int status)
static response_t *response_init();
static void response_free(response_t *response);

int response_write(client_t *client, enum http_status code, char *body, int length) {
//    DEBUG("client=%p, body(%i)=%.*s\n", client, length, length, body);
    int error = 0;
    if (code != HTTP_STATUS_OK) ERROR("code=%s\n", http_status_str(code));
//    if ((error = client->tcp.type != UV_TCP)) { FATAL("client=%p, client->tcp.type=%i\n", client, client->tcp.type); return error; }
//    if ((error = client->tcp.flags > MAX_FLAG)) { FATAL("client=%p, client->tcp.flags=%u\n", client, client->tcp.flags); return error; }
    if ((error = uv_is_closing((const uv_handle_t *)&client->tcp))) { FATAL("uv_is_closing\n"); return error; } // int uv_is_closing(const uv_handle_t* handle)
    response_t *response = response_init();
    if ((error = !response)) { FATAL("response_init\n"); return error; }
    if ((error = xbuf_xcat(&response->xbuf, STATUS, client->parser.http_major, client->parser.http_minor, http_status_str(code)) <= 0)) { FATAL("xbuf_xcat\n"); response_free(response); return error; }
    if ((error = xbuf_xcat(&response->xbuf, HEADERS, length) <= 0)) { FATAL("xbuf_xcat\n"); response_free(response); return error; }
    if ((error = xbuf_ncat(&response->xbuf, CRLF, sizeof(CRLF) - 1) <= 0)) { FATAL("xbuf_ncat\n"); response_free(response); return error; }
    if ((error = xbuf_ncat(&response->xbuf, body, length) <= 0)) { FATAL("xbuf_ncat\n"); response_free(response); return error; }
    const uv_buf_t bufs[] = {{.base = response->xbuf.base, .len = response->xbuf.len}};
    if ((error = !uv_is_writable((const uv_stream_t*)&client->tcp))) { FATAL("uv_is_writable\n"); response_free(response); /*client_close(client); */return error; } // int uv_is_writable(const uv_stream_t* handle)
    if ((error = uv_write(&response->req, (uv_stream_t *)&client->tcp, bufs, sizeof(bufs) / sizeof(bufs[0]), response_on_write))) { FATAL("uv_write\n"); response_free(response); return error; } // int uv_write(uv_write_t* req, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb)
    return error;
}

static void response_on_write(uv_write_t *req, int status) { // void (*uv_write_cb)(uv_write_t* req, int status)
//    DEBUG("req=%p, status=%i\n", req, status);
    if (status) FATAL("status=%i\n", status);
    client_t *client = (client_t *)req->handle->data;
    parser_init_or_client_close(client);
    response_t *response = (response_t *)req->data;
    response_free(response);
}

static response_t *response_init() {
    response_t *response = (response_t *)malloc(sizeof(response_t));
    if (!response) { FATAL("malloc\n"); return NULL; }
    if (!xbuf_init(&response->xbuf)) { FATAL("xbuf_init\n"); response_free(response); return NULL; }
    response->req.data = (void *)response;
    return response;
}

static void response_free(response_t *response) {
//    DEBUG("response=%p\n", response);
    xbuf_free(&response->xbuf);
    free(response);
}
