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

typedef enum state_t {STATE_IDLE, STATE_WORK} state_t;

typedef struct server_t {
//    uv_cond_t cond;
//    uv_mutex_t mutex;
    queue_t queue;
//    queue_t idle;
//    queue_t work;
} server_t;

typedef struct client_t {
    uv_tcp_t tcp;
    http_parser parser;
} client_t;

typedef struct postgres_t {
    queue_t queue;
    uv_poll_t poll;
    PGconn *conn;
    char *conninfo;
    client_t *client;
//    state_t state;
} postgres_t;

#endif // _CONTEXT_H
