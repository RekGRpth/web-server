#include "request.h" // request_*
#include "xbuffer.h" // xbuf_*
#include "macros.h" // DEBUG, ERROR, FATAL
#include <stdlib.h> // malloc, realloc, calloc, free, getenv, setenv, atoi, size_t

request_t *request_init(client_t *client) {
//    DEBUG("client=%p\n", client);
//    if (client->tcp.type != UV_TCP) { FATAL("client=%p, client->tcp.type=%i\n", client, client->tcp.type); return NULL; }
//    if (client->tcp.flags > MAX_FLAG) { FATAL("client=%p, client->tcp.flags=%u\n", client, client->tcp.flags); return NULL; }
    if (uv_is_closing((const uv_handle_t *)&client->tcp)) { FATAL("uv_is_closing\n"); return NULL; } // int uv_is_closing(const uv_handle_t* handle)
    request_t *request = (request_t *)malloc(sizeof(request_t));
    if (!request) { FATAL("malloc\n"); return NULL; }
    if (!xbuf_init(&request->xbuf)) { FATAL("xbuf_init\n"); request_free(request); return NULL; }
    request->client = client;
    request->postgres = NULL;
    pointer_init(&request->server_pointer);
    pointer_init(&request->client_pointer);
    queue_put_pointer(&client->request_queue, &request->client_pointer);
    request->argc = 0;
    request->varc = 0;
    request->hdrc = 0;
    request->bodc = 0;
//    DEBUG("request=%p, client=%p\n", request, client);
//    DEBUG("client=%p, queue_count(&client->request_queue)=%i\n", client, queue_count(&client->request_queue));
    return request;
}

void request_free(request_t *request) {
//    DEBUG("request=%p, request->client=%p, request->postgres=%p\n", request, request->client, request->postgres);
//    client_t *client = request->client; server_t *server = (server_t *)client->tcp.loop->data; DEBUG("queue_count(&server->postgres_queue)=%i, queue_count(&server->client_queue)=%i, queue_count(&server->request_queue)=%i\n", queue_count(&server->postgres_queue), queue_count(&server->client_queue), queue_count(&server->request_queue));
    pointer_remove(&request->server_pointer);
    pointer_remove(&request->client_pointer);
    if (request->postgres) if (postgres_cancel(request->postgres)) FATAL("postgres_cancel\n");
    xbuf_free(&request->xbuf);
    free(request);
}

int request_push(request_t *request) {
//    DEBUG("request=%p\n", request);
    int error = 0;
    client_t *client = request->client;
    pointer_remove(&request->server_pointer);
//    if ((error = client->tcp.type != UV_TCP)) { FATAL("client=%p, client->tcp.type=%i\n", client, client->tcp.type); return error; }
//    if ((error = client->tcp.flags > MAX_FLAG)) { FATAL("client=%p, client->tcp.flags=%u\n", client, client->tcp.flags); return error; }
    if ((error = uv_is_closing((const uv_handle_t *)&client->tcp))) { FATAL("uv_is_closing\n"); return error; } // int uv_is_closing(const uv_handle_t* handle)
    request->postgres = NULL;
    server_t *server = (server_t *)client->tcp.loop->data;
    queue_put_pointer(&server->request_queue, &request->server_pointer);
    return postgres_process(server);
}

int request_pop(request_t *request) {
//    DEBUG("request=%p\n", request);
    int error = 0;
    client_t *client = request->client;
    pointer_remove(&request->server_pointer);
//    if ((error = client->tcp.type != UV_TCP)) { FATAL("client=%p, client->tcp.type=%i\n", client, client->tcp.type); return error; }
//    if ((error = client->tcp.flags > MAX_FLAG)) { FATAL("client=%p, client->tcp.flags=%u\n", client, client->tcp.flags); return error; }
    if ((error = uv_is_closing((const uv_handle_t *)&client->tcp))) { FATAL("uv_is_closing\n"); return error; } // int uv_is_closing(const uv_handle_t* handle)
    return error;
}

