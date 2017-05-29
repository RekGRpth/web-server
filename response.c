#include <stdlib.h>
#include <uv.h>
#include "context.h"
#include "macros.h"
#include "response.h"

int response_write_response(postgres_t *postgres, char *response, int length) {
    client_t *client = (client_t *)postgres->client;
    write_req_t *wr = (write_req_t *)malloc(sizeof(write_req_t));
    if (wr == NULL) { ERROR("malloc\n"); return errno; }
    wr->req.data = wr;
#   define HEADERS \
        "Content-Type: application/json\r\n" \
        "Content-Length: %d\r\n" \
        "Connection: keep-alive\r\n"
    int headers_length = sizeof(HEADERS) - 1;
    for (int number = length; number /= 10; headers_length++);
    char headers[headers_length];
    if (snprintf(headers, headers_length, HEADERS, length) != headers_length - 1) { ERROR("snprintf\n"); return errno; }
    const uv_buf_t bufs[] = {
        {.base = "HTTP/1.1 200 OK\r\n", .len = sizeof("HTTP/1.1 200 OK\r\n") - 1},
        {.base = headers, .len = headers_length - 1},
        {.base = "\r\n", .len = sizeof("\r\n") - 1},
        {.base = response, .len = length}
    };
    if (uv_write(&wr->req, (uv_stream_t *)&client->tcp, bufs, sizeof(bufs) / sizeof(bufs[0]), response_on_write)) { ERROR("uv_write\n"); free(wr); /*return errno;*/ } // int uv_write(uv_write_t* req, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb)
    postgres->client = NULL;
    return errno;
}

void response_on_write(uv_write_t *req, int status) { // void (*uv_write_cb)(uv_write_t* req, int status)
    client_t *client = (client_t *)req->handle->data;
    if (!http_should_keep_alive(&client->parser)) request_close((uv_handle_t *)req->handle); // int http_should_keep_alive(const http_parser *parser);
    else parser_init(client);
    write_req_t *wr = (write_req_t *)req->data;
    free(wr);
}
