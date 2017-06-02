#ifndef _CLIENT_H
#define _CLIENT_H

#include "context.h"

void client_on_connect(uv_stream_t *server, int status); // void (*uv_connection_cb)(uv_stream_t* server, int status)
client_t *client_init(uv_stream_t *server);
void client_free(client_t *client);
void client_close(client_t *client);
void client_on_close(uv_handle_t *handle); // void (*uv_close_cb)(uv_handle_t* handle)
void client_on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);  // void (*uv_alloc_cb)(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf )
void client_on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf); // void (*uv_read_cb)(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf )

#endif // _CLIENT_H
