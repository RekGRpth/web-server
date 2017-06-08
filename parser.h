#ifndef _PARSER_H
#define _PARSER_H

typedef struct client_t client_t;

#include <uv.h> // uv_*

#ifdef RAGEL_HTTP_PARSER
#   include "ragel-http-parser/http_parser.h"
#else
#   include "nodejs-http-parser/http_parser.h"
#endif

#include "client.h"

void parser_init(client_t *client);
void parser_init_or_client_close(client_t *client);
int parser_should_keep_alive(client_t *client);
size_t parser_execute(client_t *client, const char *data, size_t len);
void parser_on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf); // void (*uv_alloc_cb)(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf )
void parser_on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf); // void (*uv_read_cb)(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf )
int parser_on_message_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_url(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length)
int parser_on_header_field(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length)
int parser_on_header_value(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length)
int parser_on_headers_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_body(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length)
int parser_on_message_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_chunk_header(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_chunk_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*)
#ifdef RAGEL_HTTP_PARSER
int parser_on_url_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_url_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_args_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_arg_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_arg(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length)
int parser_on_arg_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_args_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_vars_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_var_field_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_var_field(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length)
int parser_on_var_field_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_var_value_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_var_value(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length)
int parser_on_var_value_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_vars_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_headers_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_header_field_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_header_field_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_header_value_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_header_value_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_body_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*)
int parser_on_body_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*)
#endif
const char *http_status_str(enum http_status s);

#endif // _PARSER_H
