#ifndef _POSTGRES_H
#define _POSTGRES_H

#include "context.h"

#define BYTEAOID 17
#define INT8OID 20
#define INT2OID 21
#define INT4OID 23
#define TEXTOID 25
#define JSONOID 114

void postgres_on_poll(uv_poll_t *handle, int status, int events); // void (*uv_poll_cb)(uv_poll_t* handle, int status, int events)
int postgres_connect(uv_loop_t *loop, postgres_t *postgres);
int postgres_reset(postgres_t *postgres);
int postgres_push(postgres_t *postgres);
int postgres_pop(postgres_t *postgres);
int postgres_process(server_t *server);
void postgres_response(PGresult *result, postgres_t *postgres);
postgres_t *postgres_init_and_connect(uv_loop_t *loop, char *conninfo);
void postgres_free(postgres_t *postgres);
void postgres_error(PGresult *result, postgres_t *postgres);
int postgres_socket(postgres_t *postgres);
void postgres_listen(postgres_t *postgres);
int postgres_cancel(postgres_t *postgres);

#endif // _POSTGRES_H
