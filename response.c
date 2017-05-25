#include <stdlib.h>
#include <uv.h>
#include "macros.h"
#include "context.h"
#include "response.h"

#ifdef RAGEL_HTTP_PARSER
#   include "ragel-http-parser/http_parser.h"
#else
#   include "nodejs-http-parser/http_parser.h"
#endif

#define RESPONSE \
    "HTTP/1.1 200 OK\r\n" \
    "Server: libuv\r\n" \
    "Date: Thu, 21 May 2017 08:13:37 GMT\r\n" \
    "Content-Type: text/html\r\n" \
    "Content-Length: 21\r\n" \
    "Connection: keep-alive\r\n" \
    "\r\n" \
    "<p>Hello, world!</p>\n"

int response_write(client_t *client) {
    write_req_t *wr = (write_req_t *)malloc(sizeof(write_req_t));
    if (wr == NULL) {
        ERROR("malloc\n");
        return errno;
    }
    wr->req.data = wr;
    wr->buf.base = RESPONSE;
    wr->buf.len = sizeof(RESPONSE) - 1;
    if (uv_write(&wr->req, (uv_stream_t *)&client->tcp, &wr->buf, 1, response_on_write)) { // int uv_write(uv_write_t* req, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb)
        ERROR("uv_write\n");
        free(wr);
        return errno;
    }
    return errno;
}

void response_on_write(uv_write_t *req, int status) { // void (*uv_write_cb)(uv_write_t* req, int status)
    client_t *client = (client_t *)req->handle->data;
    if (!http_should_keep_alive(&client->parser)) { // int http_should_keep_alive(const http_parser *parser);
        request_close((uv_handle_t *)req->handle);
    } else {
        http_parser_init(&client->parser, HTTP_REQUEST); // void http_parser_init(http_parser *parser, enum http_parser_type type);
    }
    write_req_t *wr = (write_req_t *)req->data;
    free(wr);
}
