#ifndef _RESPONSE_H
#define _RESPONSE_H

#include <uv.h> // uv_*
#include "client.h"

typedef struct response_t {
    uv_write_t req;
} response_t;

void response_on_write(uv_write_t *req, int status); // void (*uv_write_cb)(uv_write_t* req, int status)
int response_write(client_t *client, enum http_status code, char *body, int length);
response_t *response_init();
void response_free(response_t *response);

#endif // _RESPONSE_H
