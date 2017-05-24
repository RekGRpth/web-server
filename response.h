#ifndef _RESPONSE_H
#define _RESPONSE_H

#include <uv.h>
#include "request.h"

typedef struct write_req_t {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

int write_response(context_t *context);
void on_write(uv_write_t *req, int status); // void (*uv_write_cb)(uv_write_t* req, int status)

#endif // _RESPONSE_H
