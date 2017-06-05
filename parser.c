#include "parser.h"
#include "postgres.h"
#include "client.h"
#include "request.h"

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

void parser_init(client_t *client) {
//    DEBUG("client=%p\n", client);
    http_parser_init(&client->parser, HTTP_REQUEST); // void http_parser_init(http_parser *parser, enum http_parser_type type);
}

void parser_init_or_client_close(client_t *client) {
    if (parser_should_keep_alive(client)) parser_init(client); else client_close(client);
}

int parser_should_keep_alive(client_t *client) {
//    DEBUG("client=%p\n", client);
    return http_should_keep_alive(&client->parser); // int http_should_keep_alive(const http_parser *parser);
}

size_t parser_execute(client_t *client, const char *data, size_t len) {
//    DEBUG("client=%p\n", client);
    client->parser.data = (void *)client;
    return http_parser_execute(&client->parser, (const http_parser_settings *)&parser_settings, data, len);// size_t http_parser_execute(http_parser *parser, const http_parser_settings *settings, const char *data, size_t len);
}

int parser_on_message_begin(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    int error = 0;
    client_t *client = (client_t *)parser->data;
    parser->data = (void *)request_init(client);
    if ((error = !parser->data)) { ERROR("request_init\n"); return error; }
    return error;
}

int parser_on_url(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("url(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_status(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("status(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_header_field(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("header_field(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_header_value(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("header_value(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_headers_complete(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    return 0;
}

int parser_on_body(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("body(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_message_complete(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
//    DEBUG("http_major=%i, http_minor=%i\n", parser->http_major, parser->http_minor);
//    DEBUG("content_length=%li\n", parser->content_length);
    int error = 0;
    request_t *request = (request_t *)parser->data;
    if ((error = !request)) { ERROR("no_request"); return error; }
    client_t *client = request->client;
    if ((error = client->tcp.type != UV_TCP)) { ERROR("client->tcp.type=%i\n", client->tcp.type); return error; }
    if ((error = client->tcp.flags > MAX_FLAG)) { ERROR("client->tcp.flags=%u\n", client->tcp.flags); return error; }
    if ((error = uv_is_closing((const uv_handle_t *)&client->tcp))) { ERROR("uv_is_closing\n"); return error; } // int uv_is_closing(const uv_handle_t* handle)
    if ((error = postgres_push_request(request))) { ERROR("postgres_push_request\n"); return error; }
    return error;
}

int parser_on_chunk_header(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    return 0;
}

int parser_on_chunk_complete(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    return 0;
}

#ifdef RAGEL_HTTP_PARSER
int parser_on_version(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("version(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_method(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("method(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_query(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("query(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_path(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("path(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_arg(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("arg(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_var_field(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("var_field(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_var_value(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("var_value(%li)=%.*s\n", length, (int)length, at);
    return 0;
}

int parser_on_headers_begin(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    return 0;
}
#endif
