#ifndef _RESPONSE_H
#define _RESPONSE_H

#include <uv.h>
#include "context.h"

// from request.c
void request_close(uv_handle_t *handle);

// from parser.c
void parser_init(client_t *client);
int should_keep_alive(client_t *client);

// to response.c
void response_on_write(uv_write_t *req, int status); // void (*uv_write_cb)(uv_write_t* req, int status)
int response_write(client_t *client, char *value, int length);

#endif // _RESPONSE_H
