#ifndef _CLIENT_H
#define _CLIENT_H

#include <uv.h> // uv_*
#include "queue.h" // queue_*
#include "parser.h"

enum {
  UV_CLOSING              = 0x01,   /* uv_close() called but not finished. */
  UV_CLOSED               = 0x02,   /* close(2) finished. */
  UV_STREAM_READING       = 0x04,   /* uv_read_start() called. */
  UV_STREAM_SHUTTING      = 0x08,   /* uv_shutdown() called but not complete. */
  UV_STREAM_SHUT          = 0x10,   /* Write side closed. */
  UV_STREAM_READABLE      = 0x20,   /* The stream is readable */
  UV_STREAM_WRITABLE      = 0x40,   /* The stream is writable */
  UV_STREAM_BLOCKING      = 0x80,   /* Synchronous writes. */
  UV_STREAM_READ_PARTIAL  = 0x100,  /* read(2) read less than requested. */
  UV_STREAM_READ_EOF      = 0x200,  /* read(2) read EOF. */
  UV_TCP_NODELAY          = 0x400,  /* Disable Nagle. */
  UV_TCP_KEEPALIVE        = 0x800,  /* Turn on keep-alive. */
  UV_TCP_SINGLE_ACCEPT    = 0x1000, /* Only accept() when idle. */
  UV_HANDLE_IPV6          = 0x10000, /* Handle is bound to a IPv6 socket. */
  UV_UDP_PROCESSING       = 0x20000, /* Handle is running the send callback queue. */
  UV_HANDLE_BOUND         = 0x40000  /* Handle is bound to an address and port */
};

#define MAX_FLAG (UV_CLOSING | UV_CLOSED | UV_STREAM_READING | UV_STREAM_SHUTTING | UV_STREAM_SHUT | UV_STREAM_READABLE | UV_STREAM_WRITABLE | UV_STREAM_BLOCKING | UV_STREAM_READ_PARTIAL | UV_STREAM_READ_EOF | UV_TCP_NODELAY | UV_TCP_KEEPALIVE | UV_TCP_SINGLE_ACCEPT | UV_HANDLE_IPV6 | UV_UDP_PROCESSING | UV_HANDLE_BOUND)

typedef struct client_t {
    uv_tcp_t tcp;
    http_parser parser;
    pointer_t server_pointer;
    queue_t request_queue;
    char server_ip[16];
    char client_ip[16];
    int server_port;
    int client_port;
} client_t;

void client_on_connect(uv_stream_t *stream, int status); // void (*uv_connection_cb)(uv_stream_t* server, int status)
void client_free(client_t *client);
void client_close(client_t *client);
int client_response(client_t *client, enum http_status code, char *body, int length);

#endif // _CLIENT_H
