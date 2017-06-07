#ifndef _REQUEST_H
#define _REQUEST_H

#include <uv.h> // uv_*

typedef struct postgres_t postgres_t;

#include "xbuffer.h" // xbuf_*
#include "client.h"
#include "postgres.h"

typedef struct request_t {
    client_t *client;
    postgres_t *postgres;
    pointer_t server_pointer;
    pointer_t client_pointer;
    xbuf_t xbuf;
} request_t;

request_t *request_init(client_t *client);
void request_free(request_t *request);
int request_push(request_t *request);
int request_pop(request_t *request);

#endif // _REQUEST_H
