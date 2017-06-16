#ifndef _PARSER_H
#define _PARSER_H

#ifdef RAGEL_HTTP_PARSER
#   include "ragel-http-parser/http_parser.h"
#else
#   include "nodejs-http-parser/http_parser.h"
#endif // RAGEL_HTTP_PARSER

#include "client.h" // client_t
#include <uv.h> // uv_*

void parser_init(client_t *client);
void parser_init_or_client_close(client_t *client);
size_t parser_execute(client_t *client, const char *data, size_t len);
const char *http_status_str(enum http_status s);

#endif // _PARSER_H
