#include "postgres.h"
#include "response.h"
#include "request.h"
#include "client.h"

postgres_t *postgres_init_and_connect(uv_loop_t *loop, char *conninfo) {
//    DEBUG("loop=%p, conninfo=%s\n", loop, conninfo);
    postgres_t *postgres = (postgres_t *)malloc(sizeof(postgres_t));
    if (!postgres) { ERROR("malloc\n"); return NULL; }
    postgres->conninfo = conninfo;
    postgres->request = NULL;
    pointer_init(&postgres->server_pointer);
    if (postgres_connect(loop, postgres)) { ERROR("postgres_connect\n"); postgres_free(postgres); return NULL; }
    return postgres;
}

void postgres_free(postgres_t *postgres) {
//    DEBUG("postgres=%p\n", postgres);
    pointer_remove(&postgres->server_pointer);
    PQfinish(postgres->conn);
    free(postgres);
}

int postgres_connect(uv_loop_t *loop, postgres_t *postgres) {
//    DEBUG("loop=%p, postgres=%p\n", loop, postgres);
    int error = 0;
    postgres->conn = PQconnectStart(postgres->conninfo); // PGconn *PQconnectStart(const char *conninfo)
    if ((error = !postgres->conn)) { ERROR("PQconnectStart\n"); return error; }
    if ((error = PQstatus(postgres->conn) == CONNECTION_BAD)) { ERROR("PQstatus=CONNECTION_BAD\n"); PQfinish(postgres->conn); return error; } // ConnStatusType PQstatus(const PGconn *conn)
    PQsetErrorVerbosity(postgres->conn, PQERRORS_VERBOSE); // PGVerbosity PQsetErrorVerbosity(PGconn *conn, PGVerbosity verbosity)
    uv_os_sock_t postgres_sock = PQsocket(postgres->conn);
    if ((error = postgres_sock < 0)) { ERROR("PQsocket\n"); PQfinish(postgres->conn); return error; }
    if ((error = uv_poll_init_socket(loop, &postgres->poll, postgres_sock))) { ERROR("uv_poll_init_socket\n"); PQfinish(postgres->conn); return error; } // int uv_poll_init_socket(uv_loop_t* loop, uv_poll_t* handle, uv_os_sock_t socket)
    postgres->poll.data = (void *)postgres;
    if ((error = uv_poll_start(&postgres->poll, UV_WRITABLE, postgres_on_poll))) { ERROR("uv_poll_start\n"); PQfinish(postgres->conn); return error; } // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
    return error;
}

void postgres_on_poll(uv_poll_t *handle, int status, int events) { // void (*uv_poll_cb)(uv_poll_t* handle, int status, int events)
//    DEBUG("handle=%p, status=%i, events=%i\n", handle, status, events);
    postgres_t *postgres = (postgres_t *)handle->data;
    if (status) { ERROR("status=%i\n", status); postgres_reset(postgres); return; }
    if (PQsocket(postgres->conn) < 0) { ERROR("PQsocket\n"); postgres_reset(postgres); return; } // int PQsocket(const PGconn *conn)
    switch (PQstatus(postgres->conn)) { // ConnStatusType PQstatus(const PGconn *conn)
        case CONNECTION_OK: /*DEBUG("CONNECTION_OK\n"); */break;
        case CONNECTION_BAD: ERROR("PQstatus==CONNECTION_BAD %s\n", PQerrorMessage(postgres->conn)); postgres_reset(postgres); return; // char *PQerrorMessage(const PGconn *conn)
        default: switch (PQconnectPoll(postgres->conn)) { // PostgresPollingStatusType PQconnectPoll(PGconn *conn)
            case PGRES_POLLING_FAILED: ERROR("PGRES_POLLING_FAILED\n"); postgres_reset(postgres); return;
            case PGRES_POLLING_READING: if (uv_poll_start(&postgres->poll, UV_READABLE, postgres_on_poll)) ERROR("uv_poll_start\n"); return; // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
            case PGRES_POLLING_WRITING: if (uv_poll_start(&postgres->poll, UV_WRITABLE, postgres_on_poll)) ERROR("uv_poll_start\n"); return; // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
            case PGRES_POLLING_OK: postgres_listen(postgres); break;
            default: return;
        }
    }
    if (events & UV_READABLE) {
        if (!PQconsumeInput(postgres->conn)) { ERROR("PQconsumeInput:%s\n", PQerrorMessage(postgres->conn)); postgres_reset(postgres); return; } // int PQconsumeInput(PGconn *conn); char *PQerrorMessage(const PGconn *conn)
        if (PQisBusy(postgres->conn)) return; // int PQisBusy(PGconn *conn)
        for (PGresult *result; (result = PQgetResult(postgres->conn)); PQclear(result)) switch (PQresultStatus(result)) { // PGresult *PQgetResult(PGconn *conn); void PQclear(PGresult *res); ExecStatusType PQresultStatus(const PGresult *res)
            case PGRES_TUPLES_OK: postgres_response(result, postgres); break;
            case PGRES_FATAL_ERROR: ERROR("PGRES_FATAL_ERROR\n"); postgres_error(result, postgres); break;
            default: break;
        }
        for (PGnotify *notify; (notify = PQnotifies(postgres->conn)); PQfreemem(notify)) { // PGnotify *PQnotifies(PGconn *conn); void PQfreemem(void *ptr)
            DEBUG("Asynchronous notification \"%s\" with payload \"%s\" received from server process with PID %d.\n", notify->relname, notify->extra, notify->be_pid);
        }
        if (postgres_push_postgres(postgres)) ERROR("postgres_push_postgres\n");
    }
    if (events & UV_WRITABLE) switch (PQflush(postgres->conn)) { // int PQflush(PGconn *conn);
        case 0: /*DEBUG("No data left to send\n"); */if (uv_poll_start(&postgres->poll, UV_READABLE, postgres_on_poll)) ERROR("uv_poll_start\n"); break; // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
        case 1: DEBUG("More data left to send\n"); break;
        default: ERROR("error sending query\n"); break;
    }
}

void postgres_listen(postgres_t *postgres) {
//    DEBUG("postgres=%p\n", postgres);
    if (!PQisnonblocking(postgres->conn) && PQsetnonblocking(postgres->conn, 1)) ERROR("PQsetnonblocking:%s\n", PQerrorMessage(postgres->conn)); // int PQisnonblocking(const PGconn *conn); int PQsetnonblocking(PGconn *conn, int arg); char *PQerrorMessage(const PGconn *conn)
    if (!PQsendQuery(postgres->conn, "listen \"webserver\";")) { ERROR("PQsendQuery:%s\n", PQerrorMessage(postgres->conn)); return; }// int PQsendQuery(PGconn *conn, const char *command); char *PQerrorMessage(const PGconn *conn)
    if (uv_poll_start(&postgres->poll, UV_WRITABLE, postgres_on_poll)) { ERROR("uv_poll_start\n"); return; } // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
}

int postgres_socket(postgres_t *postgres) {
//    DEBUG("postgres=%p\n", postgres);
    int error = 0;
    if ((error = PQsocket(postgres->conn) < 0)) { ERROR("PQsocket\n"); postgres_reset(postgres); return error; } // int PQsocket(const PGconn *conn)
    if ((error = PQstatus(postgres->conn) != CONNECTION_OK)) { ERROR("PQstatus!=CONNECTION_OK\n"); postgres_reset(postgres); return error; } // ConnStatusType PQstatus(const PGconn *conn)
    return error;
}

int postgres_reset(postgres_t *postgres) {
//    DEBUG("postgres=%p, postgres->request=%p\n", postgres, postgres->request);
    int error = 0;
    if (uv_is_active((uv_handle_t *)&postgres->poll)) if ((error = uv_poll_stop(&postgres->poll))) { ERROR("uv_poll_stop\n"); return error; } // int uv_is_active(const uv_handle_t* handle); int uv_poll_stop(uv_poll_t* poll)
    if (postgres->request) postgres_push_request(postgres->request);
    postgres->request = NULL;
    pointer_remove(&postgres->server_pointer);
    PQfinish(postgres->conn);
    if ((error = postgres_connect(postgres->poll.loop, postgres))) { ERROR("postgres_connect\n"); postgres_reset(postgres); return error; }
    return error;
}

void postgres_error(PGresult *result, postgres_t *postgres) {
    ERROR("\n%s\n", PQresultErrorMessage(result)); // char *PQresultErrorMessage(const PGresult *res)
}

void postgres_response(PGresult *result, postgres_t *postgres) {
//    DEBUG("result=%p, postgres=%p\n", result, postgres);
    request_t *request = postgres->request;
    if (!request) { ERROR("no_request\n"); return; }
    if (PQntuples(result) == 0 || PQnfields(result) == 0 || PQgetisnull(result, 0, 0)) { ERROR("no_data_found\n"); request_free(request); return; } // int PQntuples(const PGresult *res); int PQnfields(const PGresult *res); int PQgetisnull(const PGresult *res, int row_number, int column_number)
    client_t *client = request->client;
//    DEBUG("result=%p, postgres=%p, request=%p, client=%p\n", result, postgres, request, client);
//    if (!client) { ERROR("no_client\n"); return; }
    if (client->tcp.type != UV_TCP) { ERROR("client=%p, request=%p, client->tcp.type=%i\n", client, request, client->tcp.type); return; }
    if (client->tcp.flags > MAX_FLAG) { ERROR("client=%p, request=%p, client->tcp.flags=%u\n", client, request, client->tcp.flags); return; }
    if (uv_is_closing((const uv_handle_t *)&client->tcp)) { ERROR("uv_is_closing\n"); return; } // int uv_is_closing(const uv_handle_t* handle)
    request_free(request);
    if (response_write(client, PQgetvalue(result, 0, 0), PQgetlength(result, 0, 0))) { ERROR("response_write\n"); client_close(client); return; } // char *PQgetvalue(const PGresult *res, int row_number, int column_number); int PQgetlength(const PGresult *res, int row_number, int column_number)
}

int postgres_push_postgres(postgres_t *postgres) {
//    DEBUG("postgres=%p, postgres->request=%p\n", postgres, postgres->request);
    int error = 0;
    if ((error = postgres_socket(postgres))) { ERROR("postgres_socket\n"); return error; }
    else if ((error = PQisBusy(postgres->conn))) { ERROR("PQisBusy\n"); return error; }
    pointer_remove(&postgres->server_pointer);
    postgres->request = NULL;
    server_t *server = (server_t *)postgres->poll.loop->data;
    queue_put_pointer(&server->postgres_queue, &postgres->server_pointer);
    return postgres_process(server);
}

int postgres_pop_postgres(postgres_t *postgres) {
//    DEBUG("postgres=%p\n", postgres);
    int error = 0;
    if ((error = postgres_socket(postgres))) { ERROR("postgres_socket\n"); return error; }
    else if ((error = PQisBusy(postgres->conn))) { ERROR("PQisBusy\n"); return error; }
    pointer_remove(&postgres->server_pointer);
    return error;
}

int postgres_push_request(request_t *request) {
//    DEBUG("request=%p\n", request);
    int error = 0;
    client_t *client = request->client;
    pointer_remove(&request->server_pointer);
    if ((error = client->tcp.type != UV_TCP)) { ERROR("client=%p, client->tcp.type=%i\n", client, client->tcp.type); return error; }
    if ((error = client->tcp.flags > MAX_FLAG)) { ERROR("client=%p, client->tcp.flags=%u\n", client, client->tcp.flags); return error; }
    if ((error = uv_is_closing((const uv_handle_t *)&client->tcp))) { ERROR("uv_is_closing\n"); return error; } // int uv_is_closing(const uv_handle_t* handle)
    pointer_remove(&request->client_pointer);
    request->postgres = NULL;
    server_t *server = (server_t *)client->tcp.loop->data;
    queue_put_pointer(&server->request_queue, &request->server_pointer);
    queue_put_pointer(&client->request_queue, &request->client_pointer);
    return postgres_process(server);
}

int postgres_pop_request(request_t *request) {
//    DEBUG("request=%p\n", request);
    int error = 0;
    client_t *client = request->client;
    pointer_remove(&request->server_pointer);
    if ((error = client->tcp.type != UV_TCP)) { ERROR("client=%p, client->tcp.type=%i\n", client, client->tcp.type); return error; }
    if ((error = client->tcp.flags > MAX_FLAG)) { ERROR("client=%p, client->tcp.flags=%u\n", client, client->tcp.flags); return error; }
    if ((error = uv_is_closing((const uv_handle_t *)&client->tcp))) { ERROR("uv_is_closing\n"); return error; } // int uv_is_closing(const uv_handle_t* handle)
    pointer_remove(&request->client_pointer);
    return error;
}

int postgres_process(server_t *server) {
//    DEBUG("queue_count(&server->postgres_queue)=%i, queue_count(&server->client_queue)=%i, queue_count(&server->request_queue)=%i\n", queue_count(&server->postgres_queue), queue_count(&server->client_queue), queue_count(&server->request_queue));
    int error = 0;
    if (queue_empty(&server->postgres_queue) || queue_empty(&server->request_queue)) return error;
    request_t *request = pointer_data(queue_get_pointer(&server->request_queue), request_t, server_pointer);
    if ((error = postgres_pop_request(request))) { ERROR("postgres_pop_request\n"); return error; }
    postgres_t *postgres = pointer_data(queue_get_pointer(&server->postgres_queue), postgres_t, server_pointer);
    postgres->request = request;
    request->postgres = postgres;
    if ((error = postgres_pop_postgres(postgres))) { ERROR("postgres_pop_postgres\n"); request_free(request); return error; }
//    if ((error = response_write(request, "hi", sizeof("hi") - 1))) { ERROR("response_write\n"); request_close(request->client); } return error; // char *PQgetvalue(const PGresult *res, int row_number, int column_number); int PQgetlength(const PGresult *res, int row_number, int column_number)
//    DEBUG("request=%p\n", request);
    if ((error = !PQsendQuery(postgres->conn, "select to_json(now());"))) { ERROR("PQsendQuery:%s\n", PQerrorMessage(postgres->conn)); request_free(request); return error; } // int PQsendQuery(PGconn *conn, const char *command); char *PQerrorMessage(const PGconn *conn)
    if ((error = uv_poll_start(&postgres->poll, UV_WRITABLE, postgres_on_poll))) { ERROR("uv_poll_start\n"); request_free(request); return error; } // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
    return error;
}
