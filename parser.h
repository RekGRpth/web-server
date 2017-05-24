#ifndef _SERVER_H
#define _SERVER_H

#include <stdlib.h>
#include <uv.h>

#ifdef RAGEL_HTTP_PARSER
#   include "ragel-http-parser/http_parser.h"
#else
#   include "nodejs-http-parser/http_parser.h"
#endif

void on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf); // void (*uv_alloc_cb)(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf )
void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf); // void (*uv_read_cb)(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf )

int on_message_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*);
int on_url(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int on_status(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int on_header_field(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int on_header_value(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int on_headers_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*);
int on_body(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int on_message_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*);
int on_chunk_header(http_parser *parser); // typedef int (*http_cb) (http_parser*);
int on_chunk_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*);
#ifdef RAGEL_HTTP_PARSER
int on_version(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int on_method(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int on_query(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int on_path(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int on_arg(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int on_var_field(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int on_var_value(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int on_headers_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*);
#endif

#endif // _SERVER_H
