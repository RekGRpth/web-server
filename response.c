#include <stdlib.h> // malloc, realloc, calloc, free, getenv, setenv, atoi, size_t
#include "response.h"
#include "macros.h" // DEBUG, ERROR

#define CRLF "\r\n"

#define HEADERS \
    "Content-Type: application/json; charset=utf-8" CRLF \
    "Content-Length: %d" CRLF \
    "Connection: keep-alive" CRLF

#define STATUS "HTTP/%d.%d %s" CRLF

int response_write(client_t *client, enum http_status code, char *body, int length) {
//    DEBUG("client=%p, body(%i)=%.*s\n", client, length, length, body);
    int error = 0;
    if ((error = client->tcp.type != UV_TCP)) { ERROR("client=%p, client->tcp.type=%i\n", client, client->tcp.type); return error; }
    if ((error = client->tcp.flags > MAX_FLAG)) { ERROR("client=%p, client->tcp.flags=%u\n", client, client->tcp.flags); return error; }
    if ((error = uv_is_closing((const uv_handle_t *)&client->tcp))) { ERROR("uv_is_closing\n"); return error; } // int uv_is_closing(const uv_handle_t* handle)
    int headers_length = sizeof(HEADERS) - 1;
    for (int number = length; number /= 10; headers_length++);
    char headers[headers_length];
    if ((error = snprintf(headers, headers_length, HEADERS, length) - headers_length + 1)) { ERROR("snprintf\n"); return error; }
    int status_length = sizeof(STATUS) - 4;
    const char *status_str = http_status_str(code);
    status_length += strlen(status_str);
    char status[status_length];
    if ((error = snprintf(status, status_length, STATUS, client->parser.http_major, client->parser.http_minor, status_str) - status_length + 1)) { ERROR("snprintf:%s\n", status); return error; }
    const uv_buf_t bufs[] = {
        {.base = status, .len = status_length - 1},
        {.base = headers, .len = headers_length - 1},
        {.base = CRLF, .len = sizeof(CRLF) - 1},
        {.base = body, .len = length}
    };
    response_t *response = response_init();
    if ((error = !response)) { ERROR("response_init\n"); return error; }
    if ((error = uv_write(&response->req, (uv_stream_t *)&client->tcp, bufs, sizeof(bufs) / sizeof(bufs[0]), response_on_write))) { ERROR("uv_write\n"); response_free(response); return error; } // int uv_write(uv_write_t* req, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb)
    return error;
}

void response_on_write(uv_write_t *req, int status) { // void (*uv_write_cb)(uv_write_t* req, int status)
//    DEBUG("req=%p, status=%i\n", req, status);
    if (status) ERROR("status=%i\n", status);
    client_t *client = (client_t *)req->handle->data;
    parser_init_or_client_close(client);
    response_t *response = (response_t *)req->data;
    response_free(response);
}

response_t *response_init() {
    response_t *response = (response_t *)malloc(sizeof(response_t));
    if (!response) { ERROR("malloc\n"); return NULL; }
    response->req.data = (void *)response;
    return response;
}

void response_free(response_t *response) {
//    DEBUG("response=%p\n", response);
    free(response);
}
