#ifndef _RESPONSE_H
#define _RESPONSE_H

#include <uv.h> // uv_*
#include "xbuffer.h" // xbuf_*
#include "client.h"

typedef struct response_t {
    uv_write_t req;
    xbuf_t xbuf;
} response_t;

int response_write(client_t *client, enum http_status code, char *body, int length);

#endif // _RESPONSE_H
