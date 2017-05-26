#ifndef _REQUEST_H
#define _REQUEST_H

#include <uv.h>
#include "context.h"

// from parser.c
void parser_on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf); // void (*uv_alloc_cb)(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf )
void parser_on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf); // void (*uv_read_cb)(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf )

// from postgres.c
int postgres_connect(server_t *server);

// to request.c
void request_on_start(void *arg); // void (*uv_thread_cb)(void* arg)
void request_on_connect(uv_stream_t *server, int status); // void (*uv_connection_cb)(uv_stream_t* server, int status)
void request_close(uv_handle_t *handle);
void request_on_close(uv_handle_t *handle); // void (*uv_close_cb)(uv_handle_t* handle)

#endif // _REQUEST_H
