#ifndef _POSTGRES_H
#define _POSTGRES_H

#include <uv.h>
#include "context.h"

#define BYTEAOID 17
#define INT8OID 20
#define INT2OID 21
#define INT4OID 23
#define TEXTOID 25
#define JSONOID 114

// from request.c
void request_close(uv_handle_t *handle);

// from response.c
//int response_write_response(client_t *client, char *response, int length);
int response_write_response(postgres_t *postgres, char *response, int length);

// to postgres.c
void postgres_on_poll(uv_poll_t *handle, int status, int events); // void (*uv_poll_cb)(uv_poll_t* handle, int status, int events)
int postgres_connect(uv_loop_t *loop, postgres_t *postgres);
int postgres_reconnect(postgres_t *postgres);
void postgres_on_timer(uv_timer_t *handle); // void (*uv_timer_cb)(uv_timer_t* handle)
int postgres_query(client_t *client);
int postgres_queue(uv_loop_t *loop);
//void postgres_push(postgres_t *postgres);
//postgres_t *postgres_pop(server_t *server);
//void postgres_on_work(uv_work_t *req); // void (*uv_work_cb)(uv_work_t* req)
//void postgres_after_work(uv_work_t *req, int status); // void (*uv_after_work_cb)(uv_work_t* req, int status)

#endif // _POSTGRES_H
