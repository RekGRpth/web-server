#include "parser.h" // parser_*
#include "request.h" // request_t, request_push, request_init
#include "macros.h" // DEBUG, ERROR, FATAL

static int parser_should_keep_alive(client_t *client);
static int parser_on_message_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*)
static int parser_on_url(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length)
static int parser_on_header_field(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length)
static int parser_on_header_value(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length)
static int parser_on_headers_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*)
static int parser_on_body(http_parser *parser, const char *at, size_t length); // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length)
static int parser_on_message_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*)
//static int parser_on_chunk_header(http_parser *parser); // typedef int (*http_cb) (http_parser*)
//static int parser_on_chunk_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*)
#ifdef RAGEL_HTTP_PARSER
static int parser_on_url_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*)
static int parser_on_url_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*)
static int parser_on_headers_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*)
static int parser_on_header_field_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*)
static int parser_on_header_field_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*)
static int parser_on_header_value_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*)
static int parser_on_header_value_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*)
static int parser_on_body_begin(http_parser *parser); // typedef int (*http_cb) (http_parser*)
static int parser_on_body_complete(http_parser *parser); // typedef int (*http_cb) (http_parser*)
#endif // RAGEL_HTTP_PARSER

static const http_parser_settings parser_settings = {
    .on_message_begin = parser_on_message_begin, // http_cb
    .on_url = parser_on_url, // http_data_cb
    .on_header_field = parser_on_header_field, // http_data_cb
    .on_header_value = parser_on_header_value, // http_data_cb
    .on_headers_complete = parser_on_headers_complete, // http_cb
    .on_body = parser_on_body, // http_data_cb
    .on_message_complete = parser_on_message_complete, // http_cb
//    .on_chunk_header = parser_on_chunk_header, // http_cb
//    .on_chunk_complete = parser_on_chunk_complete, // http_cb
#ifdef RAGEL_HTTP_PARSER
    .on_url_begin = parser_on_url_begin, // http_cb
    .on_url_complete = parser_on_url_complete, // http_cb
    .on_headers_begin = parser_on_headers_begin, // http_cb
    .on_header_field_begin = parser_on_header_field_begin, // http_cb
    .on_header_field_complete = parser_on_header_field_complete, // http_cb
    .on_header_value_begin = parser_on_header_value_begin, // http_cb
    .on_header_value_complete = parser_on_header_value_complete, // http_cb
    .on_body_begin = parser_on_body_begin, // http_cb
    .on_body_complete = parser_on_body_complete, // http_cb
#endif // RAGEL_HTTP_PARSER
};

void parser_init(client_t *client) {
//    DEBUG("client=%p\n", client);
    client->parser.data = NULL;
    http_parser_init(&client->parser, HTTP_REQUEST); // void http_parser_init(http_parser *parser, enum http_parser_type type);
}

void parser_init_or_client_close(client_t *client) {
//    DEBUG("client=%p\n", client);
    if (parser_should_keep_alive(client)) parser_init(client); else client_close(client);
}

static int parser_should_keep_alive(client_t *client) {
//    DEBUG("client=%p\n", client);
    return http_should_keep_alive(&client->parser); // int http_should_keep_alive(const http_parser *parser);
}

size_t parser_execute(client_t *client, const char *data, size_t len) {
//    DEBUG("client=%p, client->parser.data=%p\n", client, client->parser.data);
    if (!client->parser.data) client->parser.data = (void *)client;
    return http_parser_execute(&client->parser, (const http_parser_settings *)&parser_settings, data, len);// size_t http_parser_execute(http_parser *parser, const http_parser_settings *settings, const char *data, size_t len);
}

static int parser_on_message_begin(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    int error = 0;
    client_t *client = (client_t *)parser->data;
    parser->data = (void *)request_init(client);
    if ((error = !parser->data)) { FATAL("request_init\n"); return error; }
    request_t *request = (request_t *)parser->data;
    if ((error = xbuf_cat(&request->info, "{") < 0)) { FATAL("xbuf_cat\n"); return error; }
//    if ((error = xbuf_xcat(&request->info, "\"Environment\":{\"Server\":\"%s:%i\",\"Client\":\"%s:%i\"}", client->server_ip, client->server_port, client->client_ip, client->client_port) < 0)) { FATAL("xbuf_xcat\n"); return error; }
    return error;
}

static int parser_on_url(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("url(%li)=%.*s\n", length, (int)length, at);
    int error = 0;
    request_t *request = (request_t *)parser->data;
    if ((error = !xbuf_ncat(&request->info, at, length))) { FATAL("xbuf_ncat\n"); return error; }
    return error;
}

static int parser_on_header_field(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("header_field(%li)=%.*s\n", length, (int)length, at);
    int error = 0;
    request_t *request = (request_t *)parser->data;
    if ((error = !xbuf_ncat(&request->info, at, length))) { FATAL("xbuf_ncat\n"); return error; }
    return error;
}

static int parser_on_header_value(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("header_value(%li)=%.*s\n", length, (int)length, at);
    int error = 0;
    request_t *request = (request_t *)parser->data;
    if ((error = !xbuf_ncat(&request->info, at, length))) { FATAL("xbuf_ncat\n"); return error; }
    return error;
}

static int parser_on_headers_complete(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    int error = 0;
    request_t *request = (request_t *)parser->data;
    if ((error = xbuf_cat(&request->info, "}") < 0)) { FATAL("xbuf_cat\n"); return error; }
    return error;
}

static int parser_on_body(http_parser *parser, const char *at, size_t length) { // typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//    DEBUG("body(%li)=%.*s\n", length, (int)length, at);
    int error = 0;
    request_t *request = (request_t *)parser->data;
    if ((error = !xbuf_ncat(&request->body, at, length))) { FATAL("xbuf_ncat\n"); return error; }
    return error;
}

static int parser_on_message_complete(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
//    DEBUG("http_major=%i, http_minor=%i\n", parser->http_major, parser->http_minor);
//    DEBUG("content_length=%li\n", parser->content_length);
    int error = 0;
    request_t *request = (request_t *)parser->data;
    if ((error = !request)) { FATAL("no_request"); return error; }
    if ((error = xbuf_cat(&request->info, "}") < 0)) { FATAL("xbuf_cat\n"); return error; }
    client_t *client = request->client;
    client->parser.data = NULL;
//    if ((error = client->tcp.type != UV_TCP)) { FATAL("client=%p, client->tcp.type=%i\n", client, client->tcp.type); return error; }
//    if ((error = client->tcp.flags > MAX_FLAG)) { FATAL("client=%p, client->tcp.flags=%u\n", client, client->tcp.flags); return error; }
//    if ((error = uv_is_closing((const uv_handle_t *)&client->tcp))) { FATAL("uv_is_closing\n"); return error; } // int uv_is_closing(const uv_handle_t* handle)
//    DEBUG("xbuf(%li)=%.*s\n", request->xbuf.len, (int)request->xbuf.len, request->xbuf.base);
    if ((error = request_push(request))) { FATAL("request_push\n"); return error; }
    return error;
}

/*static int parser_on_chunk_header(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    return 0;
}*/

/*static int parser_on_chunk_complete(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    return 0;
}*/

#ifdef RAGEL_HTTP_PARSER
static int parser_on_url_begin(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    int error = 0;
    request_t *request = (request_t *)parser->data;
    if ((error = xbuf_xcat(&request->info, "\"Server\":\"%s:%i\",\"Client\":\"%s:%i\",\"Method\":\"%s\",\"Url\":\"", request->client->server_ip, request->client->server_port, request->client->client_ip, request->client->client_port, http_method_str(parser->method)) < 0)) { FATAL("xbuf_xcat\n"); return error; }
//    if ((error = xbuf_xcat(&request->info, ",\"Method\":\"%s\"", http_method_str(parser->method)) < 0)) { FATAL("xbuf_xcat\n"); return error; }
//    if ((error = xbuf_cat(&request->info, ",\"Url\":\"") < 0)) { FATAL("xbuf_cat\n"); return error; }
    return error;
}

static int parser_on_url_complete(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    int error = 0;
    request_t *request = (request_t *)parser->data;
    if ((error = xbuf_cat(&request->info, "\"") < 0)) { FATAL("xbuf_cat\n"); return error; }
    return error;
}

static int parser_on_headers_begin(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    int error = 0;
    request_t *request = (request_t *)parser->data;
    if ((error = xbuf_xcat(&request->info, ",\"Version\":\"HTTP/%i.%i\",\"Headers\":{", parser->http_major, parser->http_minor) < 0)) { FATAL("xbuf_xcat\n"); return error; }
//    if ((error = xbuf_cat(&request->info, ",\"Headers\":{") < 0)) { FATAL("xbuf_cat\n"); return error; }
    return error;
}

static int parser_on_header_field_begin(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    int error = 0;
    request_t *request = (request_t *)parser->data;
    if ((error = xbuf_cat(&request->info, request->hdrc ? ",\"" : "\"") < 0)) { FATAL("xbuf_cat\n"); return error; }
    request->hdrc++;
    return error;
}

static int parser_on_header_field_complete(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    int error = 0;
    request_t *request = (request_t *)parser->data;
    if ((error = xbuf_cat(&request->info, "\":") < 0)) { FATAL("xbuf_cat\n"); return error; }
    return error;
}

static int parser_on_header_value_begin(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    int error = 0;
    request_t *request = (request_t *)parser->data;
    if ((error = xbuf_cat(&request->info, "\"") < 0)) { FATAL("xbuf_cat\n"); return error; }
    return error;
}

static int parser_on_header_value_complete(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    int error = 0;
    request_t *request = (request_t *)parser->data;
    if ((error = xbuf_cat(&request->info, "\"") < 0)) { FATAL("xbuf_cat\n"); return error; }
    return error;
}

static int parser_on_body_begin(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    int error = 0;
    request_t *request = (request_t *)parser->data;
    if ((error = xbuf_cat(&request->body, "") < 0)) { FATAL("xbuf_cat\n"); return error; }
    return error;
}

static int parser_on_body_complete(http_parser *parser) { // typedef int (*http_cb) (http_parser*);
//    DEBUG("\n");
    int error = 0;
    request_t *request = (request_t *)parser->data;
    if ((error = xbuf_cat(&request->body, "") < 0)) { FATAL("xbuf_cat\n"); return error; }
    return error;
}
#endif // RAGEL_HTTP_PARSER

const char *http_status_str(enum http_status s) {
    switch (s) {
#define XX(num, name, string) case HTTP_STATUS_##name: return #num " " #string;
    HTTP_STATUS_MAP(XX)
#undef XX
        default: return "<unknown>";
    }
}

int http_status_len(enum http_status s) {
    switch (s) {
#define XX(num, name, string) case HTTP_STATUS_##name: return sizeof(#num " " #string) - 1;
    HTTP_STATUS_MAP(XX)
#undef XX
        default: return sizeof("<unknown>") - 1;
    }
}
