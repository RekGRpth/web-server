#include "response.h" // response_*
#include "macros.h" // DEBUG, ERROR, FATAL
#include <stdlib.h> // malloc, realloc, calloc, free, getenv, setenv, atoi, size_t

#define CRLF "\r\n"
#define INFO "HTTP/%d.%d %s\r\nConnection: keep-alive\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: %d\r\n"

static void response_on_write(uv_write_t *req, int status); // void (*uv_write_cb)(uv_write_t* req, int status)
static response_t *response_init();
static void response_free(response_t *response);

int response_code_body(client_t *client, enum http_status code, char *body, int bodylen) {
//    DEBUG("client=%p, body(%i)=%.*s\n", client, bodylen, bodylen, body);
    int error = 0;
//    if ((error = uv_is_closing((const uv_handle_t *)&client->tcp))) { FATAL("uv_is_closing\n"); return error; } // int uv_is_closing(const uv_handle_t* handle)
    int infolen = sizeof(INFO) - 6;
    for (int number = bodylen; number /= 10; infolen++);
    const char *status_str = http_status_str(code);
    infolen += strlen(status_str);
    char info[infolen + 1];
    if ((error = snprintf(info, infolen + 1, INFO, client->parser.http_major, client->parser.http_minor, status_str, bodylen) - infolen)) { FATAL("snprintf\n"); return error; }
    return response_info_body(client, info, infolen, body, bodylen);
}

int response_info_body(client_t *client, char *info, int infolen, char *body, int bodylen) {
//    DEBUG("client=%p, info(%i)=%.*s\n", client, infolen, infolen, info);
//    DEBUG("client=%p, body(%i)=%.*s\n", client, bodylen, bodylen, body);
    int error = 0;
    if ((error = uv_is_closing((const uv_handle_t *)&client->tcp))) { FATAL("uv_is_closing\n"); return error; } // int uv_is_closing(const uv_handle_t* handle)
    response_t *response = response_init();
    if ((error = !response)) { FATAL("response_init\n"); return error; }
    if ((error = !xbuf_ncat(&response->xbuf, info, infolen))) { FATAL("xbuf_ncat\n"); response_free(response); return error; }
    if ((error = !xbuf_ncat(&response->xbuf, CRLF, sizeof(CRLF) - 1))) { FATAL("xbuf_ncat\n"); response_free(response); return error; }
    if (bodylen) if ((error = !xbuf_ncat(&response->xbuf, body, bodylen))) { FATAL("xbuf_ncat\n"); response_free(response); return error; }
    const uv_buf_t bufs[] = {{.base = response->xbuf.base, .len = response->xbuf.len}};
    if ((error = !uv_is_writable((const uv_stream_t*)&client->tcp))) { FATAL("uv_is_writable\n"); response_free(response); return error; } // int uv_is_writable(const uv_stream_t* handle)
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
