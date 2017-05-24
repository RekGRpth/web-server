#include <stdlib.h>
#include <uv.h>
#include "macros.h"
#include "request.h"
#include "response.h"
#include "parser.h"

#ifdef RAGEL_HTTP_PARSER
#   include "ragel-http-parser/http_parser.h"
#else
#   include "nodejs-http-parser/http_parser.h"
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
#ifdef RAGEL_HTTP_PARSER
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

void on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) { // void (*uv_alloc_cb)(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf )
    context_t *context = (context_t *)handle->data;
    context->parser.data = context;
    http_parser_init(&context->parser, HTTP_REQUEST); // void http_parser_init(http_parser *parser, enum http_parser_type type);
//    suggested_size = 8;
//    suggested_size = 16;
//    suggested_size = 128;
    buf->base = (char *)malloc(suggested_size);
    if (buf->base) buf->len = suggested_size;
    else ERROR("malloc\n");
}

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

int on_message_begin(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
    DEBUG("\n");
    return 0;
}

int on_url(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("url(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int on_status(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("status(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int on_header_field(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("header_field(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int on_header_value(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("header_value(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int on_headers_complete(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
    DEBUG("\n");
    return 0;
}

int on_body(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("body(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int on_message_complete(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
    DEBUG("\n");
    DEBUG("http_major=%i, http_minor=%i\n", parser->http_major, parser->http_minor);
    DEBUG("content_length=%li\n", parser->content_length);
    context_t *context = (context_t *)parser->data;
    if (write_response(context)) {
        ERROR("write_response\n");
        close_handle((uv_handle_t *)&context->client);
    }
    return errno;
}

int on_chunk_header(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
    DEBUG("\n");
    return 0;
}

int on_chunk_complete(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
    DEBUG("\n");
    return 0;
}

#ifdef RAGEL_HTTP_PARSER
int on_version(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("version(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int on_method(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("method(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int on_query(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("query(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int on_path(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("path(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int on_arg(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("arg(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int on_var_field(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("var_field(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int on_var_value(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    DEBUG("var_value(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int on_headers_begin(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
    DEBUG("\n");
    return 0;
}
#endif
