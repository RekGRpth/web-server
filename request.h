#ifndef _REQUEST_H
#define _REQUEST_H

#include <uv.h> // uv_*

typedef struct postgres_t postgres_t;

#include "xbuffer.h" // xbuf_*
#include "client.h"
#include "postgres.h"

//typedef enum state_t { STATE_NULL, STATE_BRACE_OPEN, STATE_BRACE_CLOSE, STATE_SQUARE_OPEN, STATE_SQUARE_CLOSE, STATE_FIELD, STATE_VALUE, STATE_ARG_BEGIN, STATE_ARG } state_t;

typedef struct request_t {
    client_t *client;
    postgres_t *postgres;
    pointer_t server_pointer;
    pointer_t client_pointer;
    xbuf_t xbuf;
    int argc;
    int varc;
    int hdrc;
    int bodc;
} request_t;

request_t *request_init(client_t *client);
void request_free(request_t *request);
int request_push(request_t *request);
int request_pop(request_t *request);

#endif // _REQUEST_H
