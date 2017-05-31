#include "response.h"

#define HEADERS \
    "Content-Type: application/json\r\n" \
    "Content-Length: %d\r\n" \
    "Connection: keep-alive\r\n"

int response_write(request_t *request, char *value, int length) {
    DEBUG("request=%p, value(%i)=%.*s\n", request, length, length, value);
    int error = 0;
    uv_stream_t *stream = (uv_stream_t *)&request->client->tcp;
//    if (stream->type != UV_TCP) { ERROR("stream->type=%i\n", stream->type); request_close(request->client); return -1; }
    int headers_length = sizeof(HEADERS) - 1;
    for (int number = length; number /= 10; headers_length++);
    char headers[headers_length];
    if ((error = snprintf(headers, headers_length, HEADERS, length) - headers_length + 1)) { ERROR("snprintf\n"); request_free(request); return error; }
    const uv_buf_t bufs[] = {
        {.base = "HTTP/1.1 200 OK\r\n", .len = sizeof("HTTP/1.1 200 OK\r\n") - 1},
        {.base = headers, .len = headers_length - 1},
        {.base = "\r\n", .len = sizeof("\r\n") - 1},
        {.base = value, .len = length}
    };
//    if ((error = !uv_is_writable(stream))) { ERROR("uv_is_writable\n"); request_free(request); return error; } // int uv_is_writable(const uv_stream_t* handle)
    request->req.data = request;
    if ((error = uv_write(&request->req, stream, bufs, sizeof(bufs) / sizeof(bufs[0]), response_on_write))) { ERROR("uv_write\n"); request_free(request); return error; } // int uv_write(uv_write_t* req, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb)
    return error;
}

void response_on_write(uv_write_t *req, int status) { // void (*uv_write_cb)(uv_write_t* req, int status)
    request_t *request = (request_t *)req->data;
    if (status) { ERROR("status=%i\n", status); request_free(request); return; }
    client_t *client = (client_t *)req->handle->data;
    if (!should_keep_alive(client)) request_close(client); else parser_init(client);
    request_free(request);
}
