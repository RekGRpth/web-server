#include <stdlib.h>
#include <uv.h>
#include "context.h"
#include "macros.h"
#include "parser.h"

#ifdef RAGEL_HTTP_PARSER
#   include "ragel-http-parser/http_parser.h"
#else
#   include "nodejs-http-parser/http_parser.h"
#endif

static const http_parser_settings parser_settings = {
    .on_message_begin = parser_on_message_begin, // http_cb
    .on_url = parser_on_url, // http_data_cb
    .on_status = parser_on_status, // http_data_cb
    .on_header_field = parser_on_header_field, // http_data_cb
    .on_header_value = parser_on_header_value, // http_data_cb
    .on_headers_complete = parser_on_headers_complete, // http_cb
    .on_body = parser_on_body, // http_data_cb
    .on_message_complete = parser_on_message_complete, // http_cb
    .on_chunk_header = parser_on_chunk_header, // http_cb
    .on_chunk_complete = parser_on_chunk_complete, // http_cb
#ifdef RAGEL_HTTP_PARSER
    .on_version = parser_on_version, // http_data_cb
    .on_method = parser_on_method, // http_data_cb
    .on_query = parser_on_query, // http_data_cb
    .on_path = parser_on_path, // http_data_cb
    .on_arg = parser_on_arg, // http_data_cb
    .on_var_field = parser_on_var_field, // http_data_cb
    .on_var_value = parser_on_var_value, // http_data_cb
    .on_headers_begin = parser_on_headers_begin, // http_cb
#endif
};

void parser_on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) { // void (*uv_alloc_cb)(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf )
    client_t *client = (client_t *)handle->data;
    client->parser.data = client;
    http_parser_init(&client->parser, HTTP_REQUEST); // void http_parser_init(http_parser *parser, enum http_parser_type type);
//    suggested_size = 8;
//    suggested_size = 16;
//    suggested_size = 128;
    buf->base = (char *)malloc(suggested_size);
    if (buf->base) buf->len = suggested_size;
    else ERROR("malloc\n");
}

void parser_on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) { // void (*uv_read_cb)(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf )
//    if (nread >= 0) DEBUG("stream=%p, nread=%li, buf->base=%p\n<<\n%.*s\n>>\n", stream, nread, buf->base, (int)nread, buf->base);
//    DEBUG("value=%.*s\n", (int)length, at);
    client_t *client = (client_t *)stream->data;
    if (nread == UV_EOF) { ERROR("nread=UV_EOF\n"); if (!http_should_keep_alive(&client->parser)) request_close((uv_handle_t *)&client->tcp); } // int http_should_keep_alive(const http_parser *parser);
    else if (nread < 0) { ERROR("nread=%li\n", nread); request_close((uv_handle_t *)&client->tcp); }
    else if ((ssize_t)http_parser_execute(&client->parser, (const http_parser_settings *)&parser_settings, buf->base, nread) < nread) { // size_t http_parser_execute(http_parser *parser, const http_parser_settings *settings, const char *data, size_t len);
        ERROR("http_parser_execute(%i)%s\n", HTTP_PARSER_ERRNO(&client->parser), http_errno_description(HTTP_PARSER_ERRNO(&client->parser)));
        request_close((uv_handle_t *)&client->tcp);
    }
    free(buf->base);
}

int parser_on_message_begin(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
    DEBUG("\n");
    return 0;
}

int parser_on_url(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("url(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_status(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("status(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_header_field(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("header_field(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_header_value(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("header_value(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_headers_complete(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
    DEBUG("\n");
    return 0;
}

int parser_on_body(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("body(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_message_complete(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
    DEBUG("\n");
    DEBUG("http_major=%i, http_minor=%i\n", parser->http_major, parser->http_minor);
    DEBUG("content_length=%li\n", parser->content_length);
    client_t *client = (client_t *)parser->data;
    if (postgres_query(client)) { ERROR("postgres_query\n"); return errno; }
/*    if (response_write(client)) {
        ERROR("response_write\n");
        request_close((uv_handle_t *)&client->tcp);
    }*/
    return 0;
}

int parser_on_chunk_header(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
    DEBUG("\n");
    return 0;
}

int parser_on_chunk_complete(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
    DEBUG("\n");
    return 0;
}

#ifdef RAGEL_HTTP_PARSER
int parser_on_version(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("version(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_method(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("method(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_query(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("query(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_path(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("path(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_arg(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("arg(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_var_field(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("var_field(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_var_value(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("var_value(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_headers_begin(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
    DEBUG("\n");
    return 0;
}
#endif
