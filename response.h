#ifndef _RESPONSE_H
#define _RESPONSE_H

typedef struct response_s response_t;

#include "client.h" // client_t
#include "xbuffer.h" // xbuf_*
#include <uv.h> // uv_*

struct response_s {
    uv_write_t req;
    xbuf_t xbuf;
};

int response_write(client_t *client, enum http_status code, char *body, int length);
int response_write2(client_t *client, char *info, int infolen, char *body, int bodylen);

#endif // _RESPONSE_H
