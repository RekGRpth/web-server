#ifndef _CONTEXT_H
#define _CONTEXT_H

#include <uv.h>

#ifdef RAGEL_HTTP_PARSER
#   include "ragel-http-parser/http_parser.h"
#else
#   include "nodejs-http-parser/http_parser.h"
#endif

typedef struct client_t {
    uv_tcp_t tcp;
    http_parser parser;
} client_t;

#endif // _CONTEXT_H
