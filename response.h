#ifndef _RESPONSE_H
#define _RESPONSE_H

#include "client.h" // client_t
#include "xbuffer.h" // xbuf_*
#include <uv.h> // uv_*

typedef struct response_t {
    uv_write_t req;
    xbuf_t xbuf;
} response_t;

int response_write(client_t *client, enum http_status code, char *body, int length);

#endif // _RESPONSE_H
