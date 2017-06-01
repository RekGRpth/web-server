#ifndef _CONTEXT_H
#define _CONTEXT_H

#include <postgresql/libpq-fe.h> // PQ*, PG*
#include <uv.h> // uv_*
#include "queue.h" // queue_*

#ifdef RAGEL_HTTP_PARSER
#   include "ragel-http-parser/http_parser.h"
#else
#   include "nodejs-http-parser/http_parser.h"
#endif

typedef struct server_t server_t;
typedef struct client_t client_t;
typedef struct postgres_t postgres_t;
typedef struct request_t request_t;
typedef struct response_t response_t;

typedef struct server_t {
    queue_t postgres_queue;
    queue_t request_queue;
    queue_t client_queue;
} server_t;

typedef struct client_t {
    queue_t request_queue;
    queue_t server_queue;
    uv_tcp_t tcp;
    http_parser parser;
} client_t;

typedef struct postgres_t {
    queue_t server_queue;
    uv_poll_t poll;
    PGconn *conn;
    char *conninfo;
    request_t *request;
} postgres_t;

typedef struct request_t {
    queue_t server_queue;
    queue_t client_queue;
    client_t *client;
    postgres_t *postgres;
} request_t;

typedef struct response_t {
    uv_write_t req;
} response_t;

#endif // _CONTEXT_H
