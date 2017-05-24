#ifndef _CONTEXT_H
#define _CONTEXT_H

#include <uv.h>

#ifdef RAGEL_HTTP_PARSER
#   include "ragel-http-parser/http_parser.h"
#else
#   include "nodejs-http-parser/http_parser.h"
#endif

typedef struct context_t {
    uv_tcp_t client;
    http_parser parser;
} context_t;

#endif // _CONTEXT_H
