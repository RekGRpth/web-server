#ifndef _CONTEXT_H
#define _CONTEXT_H

#include <postgresql/libpq-fe.h>
#include <uv.h>
#include "queue.h"

#ifdef RAGEL_HTTP_PARSER
#   include "ragel-http-parser/http_parser.h"
#else
#   include "nodejs-http-parser/http_parser.h"
#endif

typedef struct server_t server_t;
typedef struct client_t client_t;
typedef struct postgres_t postgres_t;

typedef struct server_t {
    queue_t postgres;
    queue_t client;
} server_t;

typedef struct client_t {
    queue_t queue;
    uv_tcp_t tcp;
    http_parser parser;
    postgres_t *postgres;
} client_t;

typedef struct postgres_t {
    queue_t queue;
    uv_poll_t poll;
    PGconn *conn;
    char *conninfo;
    client_t *client;
} postgres_t;

#endif // _CONTEXT_H
