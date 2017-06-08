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
    machine rfc2396; # https://tools.ietf.org/html/rfc2396
    lowalpha      = lower;
    upalpha       = upper;
    alphanum      = alnum; # alpha | digit
    hex           = xdigit;
    escaped       = "%" hex hex;
    mark          = "-" | "_" | "." | "!" | "~" | "*" | "'" | "(" | ")";
    unreserved    = alphanum | mark;
    reserved      = ";" | "/" | "?" | ":" | "@" | "&" | "=" | "+" | "$" | ",";
    pchar         = unreserved | escaped | ":" | "@" | "&" | "=" | "+" | "$" | ",";
    rel_segment   = ( unreserved | escaped | ";" | "@" | "&" | "=" | "+" | "$" | "," )+;
    userinfo      = ( unreserved | escaped | ";" | ":" | "&" | "=" | "+" | "$" | "," )*;
    reg_name      = ( unreserved | escaped | "$" | "," | ";" | ":" | "@" | "&" | "=" | "+" )+;
    uric_no_slash = unreserved | escaped | ";" | "?" | ":" | "@" | "&" | "=" | "+" | "$" | ",";
    uric          = reserved | unreserved | escaped;
    opaque_part   = uric_no_slash uric*;
    fragment      = uric*;
    query         = uric*;
    param         = pchar*;
    segment       = param ( ";" param )*;
    path_segments = segment ( "/" segment )*;
    abs_path      = "/"  path_segments;
    rel_path      = rel_segment abs_path?;
    port          = digit*;
    IPv4address   = digit+ "." digit+ "." digit+ "." digit+;
    toplabel      = alpha | alpha ( alphanum | "-" )* alphanum;
    domainlabel   = alphanum | alphanum ( alphanum | "-" )* alphanum;
    hostname      = ( domainlabel "." )* toplabel "."?;
    host          = hostname | IPv4address;
    hostport      = host ( ":" port )?;
    server        = ( ( userinfo "@" )? hostport )?;
    authority     = server | reg_name;
    scheme        = alpha ( alpha | digit | "+" | "-" | "." )*;
    net_path      = "//" authority abs_path?;
    hier_part     = ( net_path | abs_path ) ( "?" query )?;
    path          = ( abs_path | opaque_part )?;
    relativeURI   = ( net_path | abs_path | rel_path ) ( "?" query )?;
    absoluteURI   = scheme ":" ( hier_part | opaque_part );
    URI_reference = ( absoluteURI | relativeURI )? ( "#" fragment )?;
}%%

%%{
    machine rfc3986; # https://tools.ietf.org/html/rfc3986
    ALPHA         = alpha; # UPALPHA | LOALPHA
    DIGIT         = digit; # any US-ASCII digit "0".."9"
    HEXDIG        = xdigit; # Hexadecimal numeric characters
    sub_delims    = "!" | "$" | "&" | "'" | "(" | ")" | "*" | "+" | "," | ";" | "=";
    gen_delims    = ":" | "/" | "?" | "#" | "[" | "]" | "@";
    reserved      = gen_delims | sub_delims;
    unreserved    = ALPHA | DIGIT | "-" | "." | "_" | "~";
    pct_encoded   = "%" HEXDIG HEXDIG;
    pchar         = unreserved | pct_encoded | sub_delims | ":" | "@";
    fragment      = ( pchar | "/" | "?" )*;
    query         = ( pchar | "/" | "?" )*;
    segment       = pchar*;
    segment_nz    = pchar+;
    segment_nz_nc = ( unreserved | pct_encoded | sub_delims | "@" )+; # non-zero-length segment without any colon ":"
    path_empty    = zlen; # zero characters
    path_rootless = segment_nz ( "/" segment )*; # begins with a segment
    path_noscheme = segment_nz_nc ( "/" segment )*; # begins with a non-colon segment
    path_absolute = "/" path_rootless?; # begins with "/" but not "//"
    path_abempty  = ( "/" segment )*; # begins with "/" or is empty
    path          = path_abempty | path_absolute | path_noscheme | path_rootless | path_empty;
    reg_name      = ( unreserved | pct_encoded | sub_delims )*;
    dec_octet     = DIGIT | ( "1" .. "9" ) DIGIT | "1" DIGIT DIGIT | "2" ( "0" .. "4" ) DIGIT | "25" ( "0" .. "5" ); # 0-255
    IPv4address   = dec_octet "." dec_octet "." dec_octet "." dec_octet;
    h16           = HEXDIG{1,4};
    ls32          = ( h16 ":" h16 ) | IPv4address;
    IPv6address   =                              ( h16 ":" ){6} ls32
                  |                         "::" ( h16 ":" ){5} ls32
                  | [                 h16 ] "::" ( h16 ":" ){4} ls32
                  | [ ( h16 ":" ){1,} h16 ] "::" ( h16 ":" ){3} ls32
                  | [ ( h16 ":" ){2,} h16 ] "::" ( h16 ":" ){2} ls32
                  | [ ( h16 ":" ){3,} h16 ] "::"   h16 ":"      ls32
                  | [ ( h16 ":" ){4,} h16 ] "::"                ls32
                  | [ ( h16 ":" ){5,} h16 ] "::"                h16
                  | [ ( h16 ":" ){6,} h16 ] "::";
    IPvFuture     = "v" HEXDIG+ "." ( unreserved | sub_delims | ":" )+;
    IP_literal    = "[" ( IPv6address | IPvFuture  ) "]";
    port          = DIGIT*;
    host          = IP_literal | IPv4address | reg_name;
    userinfo      = ( unreserved | pct_encoded | sub_delims | ":" )*;
    authority     = ( userinfo "@" )? host ( ":" port )?;
    scheme        = ALPHA ( ALPHA | DIGIT | "+" | "-" | "." )*;
    relative_part = "//" authority path_abempty | path_absolute | path_noscheme | path_empty;
    relative_ref  = relative_part ( "?" query )? ( "#" fragment )?;
    hier_part     = "//" authority path_abempty | path_absolute | path_rootless | path_empty;
    absolute_URI  = scheme ":" hier_part ( "?" query )?;
    URI           = absolute_URI ( "#" fragment )?;
    URI_reference = URI | relative_ref;
}%%

%%{
    machine rfc2616; # https://tools.ietf.org/html/rfc2616
    OCTET        = any; # any 8-bit sequence of data
    CHAR         = ascii; # any US-ASCII character (octets 0 - 127)
    UPALPHA      = upper; # any US-ASCII uppercase letter "A".."Z"
    LOALPHA      = lower; # any US-ASCII lowercase letter "a".."z"
    ALPHA        = alpha; # UPALPHA | LOALPHA
    DIGIT        = digit; # any US-ASCII digit "0".."9"
    CTL          = cntrl | 127; # any US-ASCII control character (octets 0 - 31) and DEL (127)
    CR           = "\r"; # US-ASCII CR, carriage return (13)
    LF           = "\n"; # US-ASCII LF, linefeed (10)
    SP           = " "; # US-ASCII SP, space (32)
    HT           = "\t"; # US-ASCII HT, horizontal-tab (9)
    QT           = "\""; # US-ASCII double-quote mark (34)
    CRLF         = CR LF; # end-of-line marker for all protocol elements except the entity-body
    LWS          = CRLF? ( SP | HT )+; # header field values can be folded onto multiple lines if the continuation line begins with a space or horizontal tab (LWS can be replaced with single SP before interpreting)
    TEXT         = ^CTL | LWS; # any OCTET except CTLs, but including LWS (LWS must be replaced with single SP before interpreting)
    HEX          = xdigit; # Hexadecimal numeric characters
    separators   = "(" | ")" | "<" | ">" | "@" | "," | ";" | ":" | "\\" | "\"" | "/" | "[" | "]" | "?" | "=" | "{" | "}" | SP | HT;
    token        = ( CHAR - CTL - separators )+;
    ctext        = TEXT - "(" - ")"; # any TEXT excluding "(" and ")"
    qdtext       = TEXT - "\""; # any TEXT except "\""
    quoted_pair  = "\\" CHAR;
    quoted_string = "\"" (qdtext | quoted_pair )* "\"";
    HTTP_Version = "HTTP" "/" DIGIT+ "." DIGIT+;

    #http_URL     = "http:" "//" host ( ":" port )? ( abs_path ( "?" query )? )?;
    #start_line   = Request_Line | Status_Line;
    field_name   = token;
    field_content = TEXT* | (token | separators | quoted_string)*; # either *TEXT or combinations of token, separators, and quoted-string
    field_value  = ( field_content | LWS )*;
    message_header = field_name ":" field_value?;
    #generic_message = start_line (message_header CRLF)* CRLF message_body?;
    #HTTP_message = Request | Response; # HTTP/1.1 messages
}%%

%%{
    machine http_parser;
    crlf         = "\r" "\n";
    lws          = crlf? ( " " | "\t" )+;
    control      = cntrl | 127;
    safe         = "$" | "-" | "_" | ".";
    tspecials    = "(" | ")" | "<" | ">" | "@" | "," | ";" | ":" | "\\" | "\"" | "/" | "[" | "]" | "?" | "=" | "{" | "}" | " " | "\t";
    token        = ( ascii - control - tspecials )+;
    text         = ( any - control ) | lws;
#    qdtext       = text - "\"";
#    qpair        = "\\" ascii;
#    quoted       = "\"" (qdtext | qpair )* "\"";
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
//    const char *version_mark = NULL;
//    const char *method_mark = NULL;
//    const char *query_mark = NULL;
//    const char *path_mark = NULL;
    const char *arg_mark = NULL;
    const char *var_field_mark = NULL;
    const char *var_value_mark = NULL;
    if (cs == parser->url_state) url_mark = p;
    if (cs == parser->status_state) status_mark = p;
    if (cs == parser->header_field_state) header_field_mark = p;
    if (cs == parser->header_value_state) header_value_mark = p;
    if (cs == parser->body_state) body_mark = p;
//    if (cs == parser->version_state) version_mark = p;
//    if (cs == parser->method_state) method_mark = p;
//    if (cs == parser->query_state) query_mark = p;
//    if (cs == parser->path_state) path_mark = p;
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
//    CALE(version);
//    CALE(method);
//    CALE(query);
//    CALE(path);
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
