#include <string.h> // memset, NULL
#include "http_parser.h"
#include "../macros.h"

#define CALL(FOR) { if (FOR##_mark) { if (settings->on_##FOR && p - FOR##_mark > 0) { settings->on_##FOR(parser, FOR##_mark, p - FOR##_mark); } FOR##_mark = NULL; NTFY(FOR##_complete); } }
#define CALE(FOR) { if (FOR##_mark) { if (settings->on_##FOR && p - FOR##_mark > 0) { settings->on_##FOR(parser, FOR##_mark, p - FOR##_mark); } FOR##_mark = NULL; } }
#define MARK(FOR) { if (!FOR##_mark) { FOR##_mark = p; } NTFY(FOR##_begin); }
#define FILD(FOR) { if (!FOR##_field_mark) { FOR##_field_mark = p; FOR##_value_mark = NULL; } NTFY(FOR##_field_begin); }
#define VALU(FOR) { if (!FOR##_value_mark) { FOR##_value_mark = p; FOR##_field_mark = NULL; } NTFY(FOR##_value_begin); }
#define NTFY(FOR) { if (settings->on_##FOR) { settings->on_##FOR(parser); } }
#define STAT(FOR) { if (FOR##_mark) { parser->FOR##_state = cs; } }

%%{
    machine http_parser;
    crlf         = "\r" "\n";
    lws          = crlf? ( " " | "\t" )+;
    control      = cntrl | 127;
    safe         = "$" | "-" | "_" | ".";
    tspecials    = "(" | ")" | "<" | ">" | "@" | "," | ";" | ":" | "\\" | "\"" | "/" | "[" | "]" | "?" | "=" | "{" | "}" | " " | "\t";
    token        = ( ascii - control - tspecials )+;
    text         = ( any - control ) | lws;
    content      = ( any - control )* | (token | tspecials | ( "\"" ( ( any - control - "\"" ) | ( "\\" ascii ) )* "\"" ) )*;
    fragment     = token;
    arg          = token >{ MARK(arg); } ${ STAT(arg); } %{ CALL(arg); };
    args         = ( arg ( "/" arg )* "/"? ) >{ NTFY(args_begin); } %{ NTFY(args_complete); };
    field        = token - "&";
    var_field    = field >{ FILD(var); } ${ STAT(var_field); } %{ CALL(var_field); };
    var_value    = field >{ VALU(var); } ${ STAT(var_value); } %{ CALL(var_value); };
    var_null     = zlen >{ VALU(var); } ${ STAT(var_value); } %{ CALL(var_value); };
    var          = var_field ( "=" var_value | "="? var_null );
    vars         = ( var ( "&" var )* "&"? ) >{ NTFY(vars_begin); } %{ NTFY(vars_complete); };
    major        = "1" %{ parser->http_major = '1' - '0'; };
    minor        = "0" %{ parser->http_minor = '0' - '0'; } | "1" %{ parser->http_minor = '1' - '0'; };
    version      = "HTTP" "/" major "." minor;
    method       = "GET"  %{ parser->method = HTTP_GET; } | "POST" %{ parser->method = HTTP_POST; } | ( upper | digit | safe )+;
    path         = "/"? args?;
    url          = ( path ( "?" vars? )? ( "#" fragment? )? ) >{ MARK(url); } ${ STAT(url); } %{ CALL(url); };
    length       = digit >{ mark = p; } %{ if (parser->content_length > 0) { parser->content_length *= 10; } parser->content_length += (*mark - '0'); };
    header_field = token >{ if (!parser->headers_complete) { FILD(header); } } ${ STAT(header_field); } %{ CALL(header_field); };
    header_value = ( content | lws )* >{ if (!parser->headers_complete) { VALU(header); } } ${ STAT(header_value); } %{ CALL(header_value); };
    header       = ( "Connection"i ": " ( "keep-alive"i %{ parser->flags |= F_CONNECTION_KEEP_ALIVE; } | "close"i %{ parser->flags |= F_CONNECTION_CLOSE; })
                   | "Content-Length"i ": " length+
                   | header_field ": " header_value ) crlf;
    status       = ( text - "\r" - "\n" )*;
    code         = digit >{ mark = p; } %{ if (parser->status_code > 0) { parser->status_code *= 10; } parser->status_code += (*mark - '0'); };
    request      = method " " url " " version;
    status_code  = ( code{3} " " status ) >{ MARK(status); } ${ STAT(status); } %{ CALL(status); };
    response     = version " " status_code;
    start        = request | response;
    headers      = header* >{ NTFY(headers_begin); } %{ NTFY(headers_complete); parser->headers_complete = 1; };
    body         = any* >{ MARK(body); } ${ STAT(body); parser->ragel_content_length++; } %{ CALL(body); };
    message      = start crlf headers crlf body;
    main        := message >{ NTFY(message_begin); } %{ if (parser->ragel_content_length < parser->content_length) { fbreak; } else { NTFY(message_complete); } };
    write data;
}%%

void http_parser_init(http_parser *parser, enum http_parser_type type) {
    int cs = 0;
    %% write init;
    void *data = parser->data; /* preserve application data */
    memset(parser, 0, sizeof(http_parser));
    parser->data = data;
    parser->type = type;
    parser->state = cs;
}

size_t http_parser_execute(http_parser *parser, const http_parser_settings *settings, const char *data, size_t len) {
    int cs = parser->state;
    const char *p = data;
    const char *pe = data + len;
    const char *eof = pe;
    const char *mark = NULL;
    const char *url_mark = NULL;
    const char *status_mark = NULL;
    const char *header_field_mark = NULL;
    const char *header_value_mark = NULL;
    const char *body_mark = NULL;
    const char *arg_mark = NULL;
    const char *var_field_mark = NULL;
    const char *var_value_mark = NULL;
    if (cs == parser->url_state) url_mark = p;
    if (cs == parser->status_state) status_mark = p;
    if (!parser->headers_complete) { 
        if (cs == parser->header_field_state) header_field_mark = p;
        if (cs == parser->header_value_state) header_value_mark = p;
    }
    if (cs == parser->body_state) body_mark = p;
    if (cs == parser->arg_state) arg_mark = p;
    if (cs == parser->var_field_state) var_field_mark = p;
    if (cs == parser->var_value_state) var_value_mark = p;
    %% write exec;
    CALE(url);
    CALE(status);
    if (!parser->headers_complete) { 
        CALE(header_field);
        CALE(header_value);
    }
    CALE(body);
    CALE(arg);
    CALE(var_field);
    CALE(var_value);
    parser->state = cs;
    return p - data;
}

#ifndef ULLONG_MAX
#   define ULLONG_MAX ((uint64_t) -1) /* 2^64-1 */
#endif
/* Does the parser need to see an EOF to find the end of the message? */
int http_message_needs_eof(const http_parser *parser) {
    if (parser->type == HTTP_REQUEST) return 0;
    /* See RFC 2616 section 4.4 */
    if (parser->status_code / 100 == 1 || /* 1xx e.g. Continue */
        parser->status_code == 204 ||     /* No Content */
        parser->status_code == 304 ||     /* Not Modified */
        parser->flags & F_SKIPBODY) return 0;     /* response to a HEAD request */
    if ((parser->flags & F_CHUNKED) || parser->content_length != ULLONG_MAX) return 0;
    return 1;
}

int http_should_keep_alive(const http_parser *parser) {
    if (parser->http_major > 0 && parser->http_minor > 0) {
        /* HTTP/1.1 */
        if (parser->flags & F_CONNECTION_CLOSE) return 0;
    } else {
        /* HTTP/1.0 or earlier */
        if (!(parser->flags & F_CONNECTION_KEEP_ALIVE)) return 0;
    }
    return !http_message_needs_eof(parser);
}

/* Map errno values to strings for human-readable output */
#define HTTP_STRERROR_GEN(n, s) { "HPE_" #n, s },
static struct {
    const char *name;
    const char *description;
} http_strerror_tab[] = {
    HTTP_ERRNO_MAP(HTTP_STRERROR_GEN)
};
#undef HTTP_STRERROR_GEN
const char *http_errno_description(enum http_errno err) {
    return http_strerror_tab[err].description;
}

static const char *method_strings[] = {
#define XX(num, name, string) #string,
    HTTP_METHOD_MAP(XX)
#undef XX
};

const char *http_method_str(enum http_method m) {
    return ELEM_AT(method_strings, m, "<unknown>");
}
