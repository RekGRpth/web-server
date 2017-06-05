#ifndef _CONTEXT_H
#define _CONTEXT_H

#include <stdlib.h> // malloc, realloc, calloc, free, getenv, setenv, atoi, size_t
#include <postgresql/libpq-fe.h> // PQ*, PG*
#include <uv.h> // uv_*
#include "queue.h" // queue_*
#include "macros.h" // DEBUG, ERROR

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
    queue_t client_queue;
    queue_t request_queue;
} server_t;

typedef struct client_t {
    uv_tcp_t tcp;
    http_parser parser;
    pointer_t server_pointer;
    queue_t request_queue;
} client_t;

typedef struct postgres_t {
    uv_poll_t poll;
    char *conninfo;
    PGconn *conn;
    request_t *request;
    pointer_t server_pointer;
} postgres_t;

typedef struct request_t {
    client_t *client;
    postgres_t *postgres;
    pointer_t server_pointer;
    pointer_t client_pointer;
} request_t;

typedef struct response_t {
    uv_write_t req;
} response_t;

enum {
  UV_CLOSING              = 0x01,   /* uv_close() called but not finished. */
  UV_CLOSED               = 0x02,   /* close(2) finished. */
  UV_STREAM_READING       = 0x04,   /* uv_read_start() called. */
  UV_STREAM_SHUTTING      = 0x08,   /* uv_shutdown() called but not complete. */
  UV_STREAM_SHUT          = 0x10,   /* Write side closed. */
  UV_STREAM_READABLE      = 0x20,   /* The stream is readable */
  UV_STREAM_WRITABLE      = 0x40,   /* The stream is writable */
  UV_STREAM_BLOCKING      = 0x80,   /* Synchronous writes. */
  UV_STREAM_READ_PARTIAL  = 0x100,  /* read(2) read less than requested. */
  UV_STREAM_READ_EOF      = 0x200,  /* read(2) read EOF. */
  UV_TCP_NODELAY          = 0x400,  /* Disable Nagle. */
  UV_TCP_KEEPALIVE        = 0x800,  /* Turn on keep-alive. */
  UV_TCP_SINGLE_ACCEPT    = 0x1000, /* Only accept() when idle. */
  UV_HANDLE_IPV6          = 0x10000, /* Handle is bound to a IPv6 socket. */
  UV_UDP_PROCESSING       = 0x20000, /* Handle is running the send callback queue. */
  UV_HANDLE_BOUND         = 0x40000  /* Handle is bound to an address and port */
};

#define MAX_FLAG (UV_CLOSING | UV_CLOSED | UV_STREAM_READING | UV_STREAM_SHUTTING | UV_STREAM_SHUT | UV_STREAM_READABLE | UV_STREAM_WRITABLE | UV_STREAM_BLOCKING | UV_STREAM_READ_PARTIAL | UV_STREAM_READ_EOF | UV_TCP_NODELAY | UV_TCP_KEEPALIVE | UV_TCP_SINGLE_ACCEPT | UV_HANDLE_IPV6 | UV_UDP_PROCESSING | UV_HANDLE_BOUND)

#endif // _CONTEXT_H
