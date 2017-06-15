#ifndef _POSTGRES_H
#define _POSTGRES_H

typedef struct request_t request_t;

#include "server.h" // server_t
#include "request.h" // request_t
#include <postgresql/libpq-fe.h> // PQ*, PG*
#include <uv.h> // uv_*

#define BYTEAOID 17
#define INT8OID 20
#define INT2OID 21
#define INT4OID 23
#define TEXTOID 25
#define JSONOID 114

typedef struct postgres_t {
    uv_poll_t poll;
    char *conninfo;
    PGconn *conn;
    request_t *request;
    pointer_t server_pointer;
} postgres_t;

postgres_t *postgres_init_and_connect(uv_loop_t *loop, char *conninfo);
void postgres_free(postgres_t *postgres);
int postgres_cancel(postgres_t *postgres);
int postgres_process(server_t *server);

#endif // _POSTGRES_H
