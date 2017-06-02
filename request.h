#ifndef _REQUEST_H
#define _REQUEST_H

#include "context.h"

request_t *request_init(client_t *client);
void request_free(request_t *request);

#endif // _REQUEST_H
