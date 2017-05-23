#ifndef _SERVER_H
#define _SERVER_H

#include <uv.h>

#if RAGEL
#   include "ragel-http-parser/ragel_http_parser.h"
#else
#   include "http-parser/http_parser.h"
#endif

typedef struct {
    uv_tcp_t client;
    http_parser parser;
} context_t;

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

#endif // _SERVER_H
