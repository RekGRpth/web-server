#ifndef _REQUEST_H
#define _REQUEST_H

#include <uv.h>

#ifdef RAGEL_HTTP_PARSER
#   include "ragel-http-parser/http_parser.h"
#else
#   include "nodejs-http-parser/http_parser.h"
#endif

typedef struct context_t {
    uv_tcp_t client;
    http_parser parser;
} context_t;

void on_start(void *arg); // void (*uv_thread_cb)(void* arg)
void on_connect(uv_stream_t *server, int status); // void (*uv_connection_cb)(uv_stream_t* server, int status)
void close_handle(uv_handle_t * handle);
void on_close(uv_handle_t *handle); // void (*uv_close_cb)(uv_handle_t* handle)

#endif // _REQUEST_H
