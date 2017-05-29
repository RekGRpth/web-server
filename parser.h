#ifndef _PARSER_H
#define _PARSER_H

#include <stdlib.h>
#include <uv.h>
#include "context.h"

#ifdef RAGEL_HTTP_PARSER
#   include "ragel-http-parser/http_parser.h"
#else
#   include "nodejs-http-parser/http_parser.h"
#endif

// from request.c
void request_close(uv_handle_t *handle);

// from postgres.c
//int postgres_query(client_t *client);
void postgres_on_work(uv_work_t *req); // void (*uv_work_cb)(uv_work_t* req)
void postgres_after_work(uv_work_t *req, int status); // void (*uv_after_work_cb)(uv_work_t* req, int status)

// to parser.c
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
void parser_init(client_t *client);

#endif // _PARSER_H
