#ifndef _POSTGRES_H
#define _POSTGRES_H

#include <postgresql/libpq-fe.h> // PQ*, PG*
#include <uv.h> // uv_*

typedef struct request_t request_t;

#include "server.h"
#include "request.h"

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
int postgres_connect(uv_loop_t *loop, postgres_t *postgres);
void postgres_on_poll(uv_poll_t *handle, int status, int events); // void (*uv_poll_cb)(uv_poll_t* handle, int status, int events)
void postgres_listen(postgres_t *postgres);
int postgres_socket(postgres_t *postgres);
int postgres_reset(postgres_t *postgres);
void postgres_error(PGresult *result, postgres_t *postgres);
void postgres_success(PGresult *result, postgres_t *postgres);
int postgres_response(request_t *request, enum http_status code, char *body, int length);
int postgres_push(postgres_t *postgres);
int postgres_pop(postgres_t *postgres);
int postgres_cancel(postgres_t *postgres);
int postgres_process(server_t *server);
enum http_status sqlstate_code(char *sqlstate);

#endif // _POSTGRES_H
