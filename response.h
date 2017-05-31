#ifndef _RESPONSE_H
#define _RESPONSE_H

#include "context.h"

// from request.c
void request_close(client_t *client);

// from parser.c
void parser_init(client_t *client);
int should_keep_alive(client_t *client);

// to response.c
void response_on_write(uv_write_t *req, int status); // void (*uv_write_cb)(uv_write_t* req, int status)
int response_write(client_t *client, char *value, int length);
void response_free(response_t *response);

#endif // _RESPONSE_H
