#include <stdlib.h> // malloc, free
#include "response.h"
#include "macros.h"

#define HEADERS \
    "Content-Type: application/json\r\n" \
    "Content-Length: %d\r\n" \
    "Connection: keep-alive\r\n"

int response_write(client_t *client, char *value, int length) {
//    DEBUG("request=%p, value(%i)=%.*s\n", request, length, length, value);
    int error = 0;
//    client_t *client = request->client;
//    request_free(request);
    uv_stream_t *stream = (uv_stream_t *)&client->tcp;
//    if (stream->type != UV_TCP) { ERROR("stream->type=%i\n", stream->type); request_close(client); return -1; }
    if ((error = uv_is_closing((const uv_handle_t *)stream))) { ERROR("uv_is_closing\n"); return error; } // int uv_is_closing(const uv_handle_t* handle)
    int headers_length = sizeof(HEADERS) - 1;
    for (int number = length; number /= 10; headers_length++);
    char headers[headers_length];
    if ((error = snprintf(headers, headers_length, HEADERS, length) - headers_length + 1)) { ERROR("snprintf\n"); /*request_free(request); */return error; }
    const uv_buf_t bufs[] = {
        {.base = "HTTP/1.1 200 OK\r\n", .len = sizeof("HTTP/1.1 200 OK\r\n") - 1},
        {.base = headers, .len = headers_length - 1},
        {.base = "\r\n", .len = sizeof("\r\n") - 1},
        {.base = value, .len = length}
    };
//    if ((error = !uv_has_ref((const uv_handle_t *)stream))) { ERROR("uv_has_ref\n"); return error; } // int uv_has_ref(const uv_handle_t* handle)
//    if ((error = !uv_is_active((const uv_handle_t *)stream))) { ERROR("uv_is_active\n"); return error; } // int uv_is_active(const uv_handle_t* handle)
//    if ((error = uv_is_closing((const uv_handle_t *)stream))) { ERROR("uv_is_closing\n"); return error; } // int uv_is_closing(const uv_handle_t* handle)
//    if ((error = !uv_is_writable((const uv_stream_t*)stream))) { ERROR("uv_is_writable\n"); request_close(client); return error; } // int uv_is_writable(const uv_stream_t* handle)
    response_t *response = (response_t *)malloc(sizeof(response_t));
    if (!response) { ERROR("malloc\n"); request_close(client); return -1; }
    response->req.data = response;
    if ((error = uv_write(&response->req, stream, bufs, sizeof(bufs) / sizeof(bufs[0]), response_on_write))) { ERROR("uv_write\n"); request_close(client); return error; } // int uv_write(uv_write_t* req, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb)
//    request_free(request);
    return error;
}

void response_on_write(uv_write_t *req, int status) { // void (*uv_write_cb)(uv_write_t* req, int status)
    response_t *response = (response_t *)req->data;
    if (status) { ERROR("status=%i\n", status); response_free(response); return; }
    client_t *client = (client_t *)req->handle->data;
    if (!should_keep_alive(client)) request_close(client); else parser_init(client);
    response_free(response);
}

void response_free(response_t *response) {
    free(response);
}
