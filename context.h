#ifndef _CONTEXT_H
#define _CONTEXT_H

#include <postgresql/libpq-fe.h>
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

typedef struct server_t {
    uv_loop_t *loop;
    uv_poll_t poll;
    uv_timer_t timer;
    PGconn *conn;
    char *conninfo;
} server_t;

#endif // _CONTEXT_H
