#ifndef _RESPONSE_H
#define _RESPONSE_H

#include <uv.h>
#include "context.h"

typedef struct write_req_t {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

// from request.c
void request_close(uv_handle_t *handle);

// to response.c
int response_write(client_t *client);
void response_on_write(uv_write_t *req, int status); // void (*uv_write_cb)(uv_write_t* req, int status)
int response_write_response(client_t *client, char *response, int length);

#endif // _RESPONSE_H
