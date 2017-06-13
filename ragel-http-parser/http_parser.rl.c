#include <string.h> // memset, NULL
#include "http_parser.h"
#include "../macros.h"

#define INIT_DATA(FOR) const char *FOR##_mark = (cs == parser->FOR##_state ? p : NULL)
#define INIT_FIELD(FOR) INIT_DATA(FOR##_field)
#define INIT_VALUE(FOR) INIT_DATA(FOR##_value)

#define BEGIN_DATA(FOR) { if (!FOR##_mark) { FOR##_mark = p; BEGIN_NOTIFY(FOR); } }
#define BEGIN_FIELD(FOR) { if (!FOR##_field_mark) { FOR##_field_mark = p; FOR##_value_mark = NULL; BEGIN_NOTIFY(FOR##_field); } }
#define BEGIN_VALUE(FOR) { if (!FOR##_value_mark) { FOR##_value_mark = p; FOR##_field_mark = NULL; BEGIN_NOTIFY(FOR##_value); } }

#define BEGIN_NOTIFY(FOR) NOTIFY(FOR##_begin)
#define NOTIFY(FOR) { if (settings->on_##FOR) { settings->on_##FOR(parser); } }
#define COMPLETE_NOTIFY(FOR) NOTIFY(FOR##_complete)

#define STATE_DATA(FOR) { if (FOR##_mark) { parser->FOR##_state = cs; } }
#define STATE_FIELD(FOR) STATE_DATA(FOR##_field)
#define STATE_VALUE(FOR) STATE_DATA(FOR##_value)

#define COMPLETE_DATA(FOR) { if (FOR##_mark) { if (settings->on_##FOR && p - FOR##_mark > 0) { settings->on_##FOR(parser, FOR##_mark, p - FOR##_mark); } FOR##_mark = NULL; COMPLETE_NOTIFY(FOR); } }
#define COMPLETE_FIELD(FOR) COMPLETE_DATA(FOR##_field)
#define COMPLETE_VALUE(FOR) COMPLETE_DATA(FOR##_value)

#define CONTINUE_DATA(FOR) { if (FOR##_mark) { if (settings->on_##FOR && p - FOR##_mark > 0) { settings->on_##FOR(parser, FOR##_mark, p - FOR##_mark); } FOR##_mark = NULL; } }
#define CONTINUE_FIELD(FOR) CONTINUE_DATA(FOR##_field)
#define CONTINUE_VALUE(FOR) CONTINUE_DATA(FOR##_value)

%%{
    machine http_parser;
    crlf         = "\r" "\n";
    lws          = crlf? (" " | "\t")+;
    control      = cntrl | 127;
    safe         = "$" | "-" | "_" | ".";
    tspecials    = "(" | ")" | "<" | ">" | "@" | "," | ";" | ":" | "\\" | "\"" | "/" | "[" | "]" | "?" | "=" | "{" | "}" | " " | "\t";
    token        = ascii - control - tspecials;
    text         = any - control;
    fragment     = token+;
    arg          = token+ >{ BEGIN_DATA(arg); } ${ STATE_DATA(arg); } %{ COMPLETE_DATA(arg); };
    args         = (arg ("/" arg)* "/"?) >{ BEGIN_NOTIFY(args); } %{ COMPLETE_NOTIFY(args); };
    field        = token - "&";
    var_field    = field+ >{ BEGIN_FIELD(var); } ${ STATE_FIELD(var); } %{ COMPLETE_FIELD(var); };
    var_value    = field+ >{ BEGIN_VALUE(var); } ${ STATE_VALUE(var); } %{ COMPLETE_VALUE(var); };
    var_null     = zlen >{ BEGIN_VALUE(var); } ${ STATE_VALUE(var); } %{ COMPLETE_VALUE(var); };
    var          = var_field (("=" var_value) | ("="? var_null));
    vars         = (var ("&" var)* "&"?) >{ BEGIN_NOTIFY(vars); } %{ COMPLETE_NOTIFY(vars); };
    major        = "1" %{ parser->http_major = '1' - '0'; };
    minor        = "0" %{ parser->http_minor = '0' - '0'; } | "1" %{ parser->http_minor = '1' - '0'; };
    version      = "HTTP/" major "." minor;
    method       = "GET" %{ parser->method = HTTP_GET; } | "POST" %{ parser->method = HTTP_POST; } | (upper | digit | safe)+;
    path         = "/"? args?;
    url          = (path ("?" vars?)? ("#" fragment?)?) >{ BEGIN_DATA(url); } ${ STATE_DATA(url); } %{ COMPLETE_DATA(url); };
    length       = digit >{ mark = p; } %{ if (parser->content_length > 0) { parser->content_length *= 10; } parser->content_length += (*mark - '0'); };
    header_field = token+ >{ BEGIN_FIELD(header); } ${ STATE_FIELD(header); } %{ COMPLETE_FIELD(header); };
    header_value = text* >{ BEGIN_VALUE(header); } ${ STATE_VALUE(header); } %{ COMPLETE_VALUE(header); };
    header       = ("Connection"i ": " ("keep-alive"i %{ parser->flags |= F_CONNECTION_KEEP_ALIVE; } | "close"i %{ parser->flags |= F_CONNECTION_CLOSE; })
                   | "Content-Length"i ": " length+
                   | header_field ": " header_value) crlf;
    request      = method " " url " " version;
    headers      = header* >{ BEGIN_NOTIFY(headers); } %{ COMPLETE_NOTIFY(headers); parser->headers_complete = 1; };
    body_field   = field+ >{ BEGIN_FIELD(body); } ${ STATE_FIELD(body); } %{ COMPLETE_FIELD(body); };
    body_value   = field+ >{ BEGIN_VALUE(body); } ${ STATE_VALUE(body); } %{ COMPLETE_VALUE(body); };
    body_null    = zlen >{ BEGIN_VALUE(body); } ${ STATE_VALUE(body); } %{ COMPLETE_VALUE(body); };
    body1        = body_field (("=" body_value) | ("="? body_null));
    body2        = (body1 ("&" body1)* "&"?) >{ BEGIN_NOTIFY(body); } ${ STATE_DATA(body); parser->ragel_content_length++; } %{ COMPLETE_NOTIFY(body); };
    body3        = zlen >{ BEGIN_NOTIFY(body); } %{ COMPLETE_NOTIFY(body); };
#    body         = any+ >{ BEGIN_DATA(body); } ${ STATE_DATA(body); parser->ragel_content_length++; } %{ COMPLETE_DATA(body); };
    message      = request crlf headers crlf ( body2 | body3 );
    main        := message >{ BEGIN_NOTIFY(message); } %{ if (parser->ragel_content_length < parser->content_length) { fbreak; } else { COMPLETE_NOTIFY(message); } };
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
    INIT_DATA(url);
    INIT_DATA(arg);
    INIT_FIELD(var);
    INIT_VALUE(var);
    INIT_FIELD(header);
    INIT_VALUE(header);
    INIT_DATA(body);
    INIT_FIELD(body);
    INIT_VALUE(body);
    %% write exec;
    CONTINUE_DATA(url);
    CONTINUE_DATA(arg);
    CONTINUE_FIELD(var);
    CONTINUE_VALUE(var);
    if (!parser->headers_complete) CONTINUE_FIELD(header);
    if (!parser->headers_complete) CONTINUE_VALUE(header);
    CONTINUE_DATA(body);
    CONTINUE_FIELD(body);
    CONTINUE_VALUE(body);
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
