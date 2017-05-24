#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "macros.h"
#include "server.h"

#if RAGEL
#   include "ragel-http-parser/ragel_http_parser.h"
#else
#   include "http-parser/http_parser.h"
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

void context_del(context_t *context) {
    free(context);
}

void on_close(uv_handle_t *handle) { // void (*uv_close_cb)(uv_handle_t* handle)
    context_t *context = (context_t *)handle->data;
    context_del(context);
}

void close_handle(uv_handle_t * handle) {
    if (!uv_is_closing(handle)) { // int uv_is_closing(const uv_handle_t* handle)
        uv_close(handle, on_close); // void uv_close(uv_handle_t* handle, uv_close_cb close_cb)
    }
}

void on_write(uv_write_t *req, int status) { // void (*uv_write_cb)(uv_write_t* req, int status)
    write_req_t *wr = (write_req_t *)req->data;
    context_t *context = (context_t *)req->handle->data;
//    DEBUG("http_should_keep_alive=%i\n", http_should_keep_alive(&context->parser));
    if (!http_should_keep_alive(&context->parser)) { // int http_should_keep_alive(const http_parser *parser);
        close_handle((uv_handle_t *)req->handle);
    } else {
//        DEBUG("reinit\n");
        http_parser_init(&context->parser, HTTP_REQUEST); // void http_parser_init(http_parser *parser, enum http_parser_type type);
    }
    free(wr);
}

int write_response(context_t *context) {
    write_req_t *wr = (write_req_t *)malloc(sizeof(write_req_t));
    if (wr == NULL) {
        ERROR("malloc\n");
//        close_handle((uv_handle_t *)&context->client);
        return errno;
    }
    wr->req.data = wr;
    wr->buf = uv_buf_init(RESPONSE, sizeof(RESPONSE) - 1); // uv_buf_t uv_buf_init(char* base, unsigned int len)
    if (uv_write(&wr->req, (uv_stream_t *)&context->client, &wr->buf, 1, on_write)) { // int uv_write(uv_write_t* req, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb)
        ERROR("uv_write\n");
        free(wr);
//        close_handle((uv_handle_t *)&context->client);
        return errno;
    }
    return 0;
}

int on_message_begin(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    return 0;
}
int on_url(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("url(%li)=%.*s\n", length, (int)length, at);
    return 0;
}
int on_status(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("status(%li)=%.*s\n", length, (int)length, at);
    return 0;
}
int on_header_field(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("header_field(%li)=%.*s\n", length, (int)length, at);
    return 0;
}
int on_header_value(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("header_value(%li)=%.*s\n", length, (int)length, at);
    return 0;
}
int on_headers_complete(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    return 0;
}
int on_body(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("body(%li)=%.*s\n", length, (int)length, at);
    return 0;
}
int on_message_complete(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
//    DEBUG("http_major=%i, http_minor=%i\n", parser->http_major, parser->http_minor);
//    DEBUG("content_length=%li\n", parser->content_length);
    context_t *context = (context_t *)parser->data;
    if (write_response(context)) {
        ERROR("write_response\n");
        close_handle((uv_handle_t *)&context->client);
    }
    return errno;
}
int on_chunk_header(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    return 0;
}
int on_chunk_complete(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    return 0;
}
#if RAGEL
int on_version(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("version(%li)=%.*s\n", length, (int)length, at);
    return 0;
}
int on_method(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("method(%li)=%.*s\n", length, (int)length, at);
    return 0;
}
int on_query(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("query(%li)=%.*s\n", length, (int)length, at);
    return 0;
}
int on_path(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("path(%li)=%.*s\n", length, (int)length, at);
    return 0;
}
int on_arg(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("arg(%li)=%.*s\n", length, (int)length, at);
    return 0;
}
int on_var_field(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("var_field(%li)=%.*s\n", length, (int)length, at);
    return 0;
}
int on_var_value(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("var_value(%li)=%.*s\n", length, (int)length, at);
    return 0;
}
int on_headers_begin(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    return 0;
}
#endif

static const http_parser_settings parser_settings = {
    .on_message_begin = on_message_begin, // http_cb
    .on_url = on_url, // http_data_cb
    .on_status = on_status, // http_data_cb
    .on_header_field = on_header_field, // http_data_cb
    .on_header_value = on_header_value, // http_data_cb
    .on_headers_complete = on_headers_complete, // http_cb
    .on_body = on_body, // http_data_cb
    .on_message_complete = on_message_complete, // http_cb
    .on_chunk_header = on_chunk_header, // http_cb
    .on_chunk_complete = on_chunk_complete, // http_cb
#if RAGEL
    .on_version = on_version, // http_data_cb
    .on_method = on_method, // http_data_cb
    .on_query = on_query, // http_data_cb
    .on_path = on_path, // http_data_cb
    .on_arg = on_arg, // http_data_cb
    .on_var_field = on_var_field, // http_data_cb
    .on_var_value = on_var_value, // http_data_cb
    .on_headers_begin = on_headers_begin, // http_cb
#endif
};

void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) { // void (*uv_read_cb)(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf )
//    if (nread >= 0) DEBUG("stream=%p, nread=%li, buf->base=%p\n<<\n%.*s\n>>\n", stream, nread, buf->base, (int)nread, buf->base);
//    DEBUG("value=%.*s\n", (int)length, at);
    context_t *context = (context_t *)stream->data;
    if (nread == UV_EOF) {
//        ERROR("nread=UV_EOF\n");
        if (!http_should_keep_alive(&context->parser)) { // int http_should_keep_alive(const http_parser *parser);
            close_handle((uv_handle_t *)&context->client);
        }
    } else if (nread < 0) {
        ERROR("nread=%li\n", nread);
        close_handle((uv_handle_t *)&context->client);
    } else if ((ssize_t)http_parser_execute(&context->parser, (const http_parser_settings *)&parser_settings, buf->base, nread) < nread) { // size_t http_parser_execute(http_parser *parser, const http_parser_settings *settings, const char *data, size_t len);
        ERROR("http_parser_execute(%i)%s\n", HTTP_PARSER_ERRNO(&context->parser), http_errno_description(HTTP_PARSER_ERRNO(&context->parser)));
        close_handle((uv_handle_t *)&context->client);
    }
    free(buf->base);
}

void on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) { // void (*uv_alloc_cb)(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf )
//    suggested_size = 8;
//    suggested_size = 16;
//    suggested_size = 128;
    buf->base = (char *)malloc(suggested_size);
    if (buf->base) buf->len = suggested_size;
    else ERROR("malloc\n");
}

context_t *context_new() {
    context_t *context = (context_t *)malloc(sizeof(context_t));
    if (context == NULL) {
        ERROR("malloc\n");
        return NULL;
    }
    return context;
}

void on_connect(uv_stream_t *server, int status) { // void (*uv_connection_cb)(uv_stream_t* server, int status)
//    DEBUG("server=%p, status=%i\n", server, status);
    if (status) {
        ERROR("status");
        return;
    }
    context_t *context = context_new();
//    context_t *context = (context_t *)malloc(sizeof(context_t));
    if (context == NULL) {
        ERROR("context_new\n");
        return;
    }
    if (uv_tcp_init(server->loop, &context->client)) { // int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle)
        ERROR("uv_tcp_init\n");
        context_del(context);
        return;
    }
    context->client.data = context;
    context->parser.data = context;
    http_parser_init(&context->parser, HTTP_REQUEST); // void http_parser_init(http_parser *parser, enum http_parser_type type);
    if (uv_accept(server, (uv_stream_t *)&context->client)) { // int uv_accept(uv_stream_t* server, uv_stream_t* client)
        ERROR("uv_accept\n");
        close_handle((uv_handle_t *)&context->client);
        return;
    }
    if (uv_read_start((uv_stream_t *)&context->client, on_alloc, on_read)) { // int uv_read_start(uv_stream_t* stream, uv_alloc_cb alloc_cb, uv_read_cb read_cb)
        ERROR("uv_read_start\n");
        close_handle((uv_handle_t *)&context->client);
        return;
    }
}

#define IP "0.0.0.0"
#define PORT 8080
#define BACKLOG 128

void on_start(void *arg) { // void (*uv_thread_cb)(void* arg)
    uv_loop_t loop;
    if (uv_loop_init(&loop)) { // int uv_loop_init(uv_loop_t* loop)
        ERROR("uv_loop_init\n");
        return;
    }
    uv_tcp_t handle;
    if (uv_tcp_init(&loop, &handle)) { // int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle)
        ERROR("uv_tcp_init\n");
        return;
    }
    if (uv_tcp_open(&handle, *((uv_os_sock_t *)arg))) { // int uv_tcp_open(uv_tcp_t* handle, uv_os_sock_t sock)
        ERROR("uv_tcp_open\n");
        return;
    }
    if (uv_listen((uv_stream_t *)&handle, BACKLOG, on_connect)) { // int uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb)
        ERROR("uv_listen\n");
        return;
    }
    if (uv_run(&loop, UV_RUN_DEFAULT)) { // int uv_run(uv_loop_t* loop, uv_run_mode mode)
        ERROR("uv_run\n");
        return;
    }
}

#define THREADS 8

int main(int argc, char **argv) {
//    DEBUG("%li\n", sizeof("<p>Hello, world!</p>\n") - 1);
//    DEBUG("SOMAXCONN=%i\n", SOMAXCONN);
    if (uv_replace_allocator(malloc, realloc, calloc, free)) { // int uv_replace_allocator(uv_malloc_func malloc_func, uv_realloc_func realloc_func, uv_calloc_func calloc_func, uv_free_func free_func)
        ERROR("uv_replace_allocator\n");
        return errno;
    }
    uv_loop_t loop;
    if (uv_loop_init(&loop)) { // int uv_loop_init(uv_loop_t* loop)
        ERROR("uv_loop_init\n");
        return errno;
    }
    uv_tcp_t handle;
    if (uv_tcp_init(&loop, &handle)) { // int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle)
        ERROR("uv_tcp_init\n");
        return errno;
    }
    struct sockaddr_in addr;
    if (uv_ip4_addr(IP, PORT, &addr)) { // int uv_ip4_addr(const char* ip, int port, struct sockaddr_in* addr)
        ERROR("uv_ip4_addr IP=%s, PORT=%i\n", IP, PORT);
        return errno;
    }
    if (uv_tcp_bind(&handle, (const struct sockaddr *)&addr, 0)) { // int uv_tcp_bind(uv_tcp_t* handle, const struct sockaddr* addr, unsigned int flags)
        ERROR("uv_tcp_bind\n");
        return errno;
    }
    uv_thread_t tid[THREADS];
    for (int i = 0; i < THREADS; i++) {
        if (uv_thread_create(&tid[i], on_start, (void *)&handle.io_watcher.fd)) { // int uv_thread_create(uv_thread_t* tid, uv_thread_cb entry, void* arg)
            ERROR("uv_thread_create\n");
            return errno;
        }
    }
    for (int i = 0; i < THREADS; i++) {
        if (uv_thread_join(&tid[i])) { // int uv_thread_join(uv_thread_t *tid)
            ERROR("uv_thread_join\n");
            return errno;
        }
    }
    if (uv_run(&loop, UV_RUN_DEFAULT)) { // int uv_run(uv_loop_t* loop, uv_run_mode mode)
        ERROR("uv_run\n");
        return errno;
    }
    return errno;
}
