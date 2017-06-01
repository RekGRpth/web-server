#ifndef _PARSER_H
#define _PARSER_H

#include "context.h"

// from request.c
void request_close(client_t *client);
void request_free(request_t *request);
request_t *request_init(client_t *client);

// from postgres.c
int postgres_push_request(request_t *request);

// to parser.c
void parser_init(client_t *client);
int parser_should_keep_alive(client_t *client);
size_t parser_execute(client_t *client, const char *data, size_t len);
void parser_on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf); // void (*uv_alloc_cb)(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf )
void parser_on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf); // void (*uv_read_cb)(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf )
int parser_on_message_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*);
int parser_on_url(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int parser_on_status(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int parser_on_header_field(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int parser_on_header_value(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int parser_on_headers_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*);
int parser_on_body(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int parser_on_message_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*);
int parser_on_chunk_header(http_parser *parser); // typedef int (*http_cb) (http_parser*);
int parser_on_chunk_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*);
#ifdef RAGEL_HTTP_PARSER
int parser_on_version(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int parser_on_method(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int parser_on_query(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int parser_on_path(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int parser_on_arg(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int parser_on_var_field(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int parser_on_var_value(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
int parser_on_headers_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*);
#endif

#endif // _PARSER_H
