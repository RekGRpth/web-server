#include "response.h"
#include "parser.h"

#define HEADERS \
    "Content-Type: application/json\r\n" \
    "Content-Length: %d\r\n" \
    "Connection: keep-alive\r\n"

int response_write(client_t *client, char *value, int length) {
    DEBUG("client=%p, value(%i)=%.*s\n", client, length, length, value);
    int error = 0;
//    if ((error = client->tcp.type != UV_TCP)) { ERROR("client->tcp.type=%i\n", client->tcp.type); return error; }
//    if ((error = uv_is_closing((const uv_handle_t *)&client->tcp))) { ERROR("uv_is_closing\n"); return error; } // int uv_is_closing(const uv_handle_t* handle)
    int headers_length = sizeof(HEADERS) - 1;
    for (int number = length; number /= 10; headers_length++);
    char headers[headers_length];
    if ((error = snprintf(headers, headers_length, HEADERS, length) - headers_length + 1)) { ERROR("snprintf\n"); return error; }
    const uv_buf_t bufs[] = {
        {.base = "HTTP/1.1 200 OK\r\n", .len = sizeof("HTTP/1.1 200 OK\r\n") - 1},
        {.base = headers, .len = headers_length - 1},
        {.base = "\r\n", .len = sizeof("\r\n") - 1},
        {.base = value, .len = length}
    };
    response_t *response = response_init();
    if ((error = !response)) { ERROR("response_init\n"); return error; }
    if ((error = uv_write(&response->req, (uv_stream_t *)&client->tcp, bufs, sizeof(bufs) / sizeof(bufs[0]), response_on_write))) { ERROR("uv_write\n"); response_free(response); return error; } // int uv_write(uv_write_t* req, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb)
    return error;
}

void response_on_write(uv_write_t *req, int status) { // void (*uv_write_cb)(uv_write_t* req, int status)
    DEBUG("req=%p, status=%i\n", req, status);
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
    DEBUG("response=%p\n", response);
    free(response);
}
