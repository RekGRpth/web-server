#ifndef _POSTGRES_H
#define _POSTGRES_H

#include "context.h"

#define BYTEAOID 17
#define INT8OID 20
#define INT2OID 21
#define INT4OID 23
#define TEXTOID 25
#define JSONOID 114

// from request.c
void request_close(client_t *client);
void request_free(request_t *request);

// from response.c
int response_write(client_t *client, char *value, int length);

// to postgres.c
void postgres_on_poll(uv_poll_t *handle, int status, int events); // void (*uv_poll_cb)(uv_poll_t* handle, int status, int events)
int postgres_connect(uv_loop_t *loop, postgres_t *postgres);
int postgres_reconnect(postgres_t *postgres);
void postgres_on_timer(uv_timer_t *handle); // void (*uv_timer_cb)(uv_timer_t* handle)
int postgres_queue(uv_loop_t *loop);
int postgres_push_postgres(postgres_t *postgres);
int postgres_pop_postgres(postgres_t *postgres);
int postgres_process(server_t *server);
void postgres_response(PGresult *result, postgres_t *postgres);
int postgres_push_request(request_t *request);
int postgres_pop_request(request_t *request);
//int postgres_push_client(client_t *client);
postgres_t *postgres_init(uv_loop_t *loop, char *conninfo);
void postgres_free(postgres_t *postgres);
uv_timer_t *postgres_timer_init(postgres_t *postgres);
void postgres_timer_free(uv_timer_t *timer);

#endif // _POSTGRES_H
