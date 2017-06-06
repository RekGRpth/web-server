#ifndef _RESPONSE_H
#define _RESPONSE_H

#include "context.h"

void response_on_write(uv_write_t *req, int status); // void (*uv_write_cb)(uv_write_t* req, int status)
int response_write(client_t *client, char *body, int length);
response_t *response_init();
void response_free(response_t *response);

#endif // _RESPONSE_H
