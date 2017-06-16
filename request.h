#ifndef _REQUEST_H
#define _REQUEST_H

typedef struct request_s request_t;

#include "client.h" // client_t
#include "postgres.h" // postgres_t
#include "xbuffer.h" // xbuf_*
#include <uv.h> // uv_*

struct request_s {
    client_t *client;
    postgres_t *postgres;
    pointer_t server_pointer;
    pointer_t client_pointer;
    xbuf_t info;
    xbuf_t body;
//    int argc;
//    int varc;
    int hdrc;
//    int bodc;
};

request_t *request_init(client_t *client);
void request_free(request_t *request);
int request_push(request_t *request);
int request_pop(request_t *request);

#endif // _REQUEST_H
