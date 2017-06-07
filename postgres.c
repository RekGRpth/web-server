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
//    PQsetErrorVerbosity(postgres->conn, PQERRORS_VERBOSE); // PGVerbosity PQsetErrorVerbosity(PGconn *conn, PGVerbosity verbosity)
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
//    DEBUG("PQstatus(postgres->conn)=%i\n", PQstatus(postgres->conn));
    switch (PQstatus(postgres->conn)) { // ConnStatusType PQstatus(const PGconn *conn)
        case CONNECTION_OK: /*DEBUG("CONNECTION_OK\n"); */break;
        case CONNECTION_BAD: ERROR("PQstatus==CONNECTION_BAD %s", PQerrorMessage(postgres->conn)); postgres_reset(postgres); return; // char *PQerrorMessage(const PGconn *conn)
        default: switch (PQconnectPoll(postgres->conn)) { // PostgresPollingStatusType PQconnectPoll(PGconn *conn)
            case PGRES_POLLING_FAILED: ERROR("PGRES_POLLING_FAILED\n"); postgres_reset(postgres); return;
            case PGRES_POLLING_READING: if (uv_poll_start(&postgres->poll, UV_READABLE, postgres_on_poll)) ERROR("uv_poll_start\n"); return; // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
            case PGRES_POLLING_WRITING: if (uv_poll_start(&postgres->poll, UV_WRITABLE, postgres_on_poll)) ERROR("uv_poll_start\n"); return; // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
            case PGRES_POLLING_OK: postgres_listen(postgres); break;
            default: return;
        }
    }
    if (events & UV_READABLE) {
        if (!PQconsumeInput(postgres->conn)) { ERROR("PQconsumeInput:%s", PQerrorMessage(postgres->conn)); postgres_reset(postgres); return; } // int PQconsumeInput(PGconn *conn); char *PQerrorMessage(const PGconn *conn)
        if (PQisBusy(postgres->conn)) return; // int PQisBusy(PGconn *conn)
        for (PGresult *result; (result = PQgetResult(postgres->conn)); PQclear(result)) switch (PQresultStatus(result)) { // PGresult *PQgetResult(PGconn *conn); void PQclear(PGresult *res); ExecStatusType PQresultStatus(const PGresult *res)
            case PGRES_TUPLES_OK: postgres_success(result, postgres); break;
            case PGRES_FATAL_ERROR: ERROR("PGRES_FATAL_ERROR\n"); postgres_error(result, postgres); break;
            default: break;
        }
        for (PGnotify *notify; (notify = PQnotifies(postgres->conn)); PQfreemem(notify)) { // PGnotify *PQnotifies(PGconn *conn); void PQfreemem(void *ptr)
            DEBUG("Asynchronous notification \"%s\" with payload \"%s\" received from server process with PID %d.\n", notify->relname, notify->extra, notify->be_pid);
        }
        if (postgres_push(postgres)) ERROR("postgres_push\n");
    }
    if (events & UV_WRITABLE) switch (PQflush(postgres->conn)) { // int PQflush(PGconn *conn);
        case 0: /*DEBUG("No data left to send\n"); */if (uv_poll_start(&postgres->poll, UV_READABLE, postgres_on_poll)) ERROR("uv_poll_start\n"); break; // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
        case 1: DEBUG("More data left to send\n"); break;
        default: ERROR("error sending query\n"); break;
    }
}

void postgres_listen(postgres_t *postgres) {
//    DEBUG("postgres=%p\n", postgres);
    if (!PQisnonblocking(postgres->conn) && PQsetnonblocking(postgres->conn, 1)) ERROR("PQsetnonblocking:%s", PQerrorMessage(postgres->conn)); // int PQisnonblocking(const PGconn *conn); int PQsetnonblocking(PGconn *conn, int arg); char *PQerrorMessage(const PGconn *conn)
    if (!PQsendQuery(postgres->conn, "listen \"webserver\";")) { ERROR("PQsendQuery:%s", PQerrorMessage(postgres->conn)); return; }// int PQsendQuery(PGconn *conn, const char *command); char *PQerrorMessage(const PGconn *conn)
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
    if (postgres->request) if (request_push(postgres->request)) ERROR("request_push\n");
    postgres->request = NULL;
    pointer_remove(&postgres->server_pointer);
//    PQfinish(postgres->conn);
//    if ((error = postgres_connect(postgres->poll.loop, postgres))) { ERROR("postgres_connect\n"); postgres_reset(postgres); return error; }
    if ((error = !PQresetStart(postgres->conn))) { ERROR("PQresetStart\n"); return error; } // int PQresetStart(PGconn *conn);
    postgres->poll.io_watcher.fd = PQsocket(postgres->conn);
    if ((error = uv_poll_start(&postgres->poll, UV_WRITABLE, postgres_on_poll))) { ERROR("uv_poll_start\n"); return error; } // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
    return error;
}

void postgres_error(PGresult *result, postgres_t *postgres) {
    char *message = PQresultErrorMessage(result); // char *PQresultErrorMessage(const PGresult *res)
    ERROR("%s", message);
    if (postgres_socket(postgres)) { ERROR("postgres_socket\n"); return; }
    request_t *request = postgres->request;
    if (postgres_response(request, sqlstate_code(PQresultErrorField(result, PG_DIAG_SQLSTATE)), message, strlen(message))) ERROR("postgres_response\n"); // char *PQresultErrorField(const PGresult *res, int fieldcode)
}

void postgres_success(PGresult *result, postgres_t *postgres) {
//    DEBUG("result=%p, postgres=%p\n", result, postgres);
    request_t *request = postgres->request;
    if (PQntuples(result) == 0 || PQnfields(result) == 0 || PQgetisnull(result, 0, 0)) { ERROR("no_data_found\n"); if (request) request_free(request); return; } // int PQntuples(const PGresult *res); int PQnfields(const PGresult *res); int PQgetisnull(const PGresult *res, int row_number, int column_number)
    if (postgres_response(request, HTTP_STATUS_OK, PQgetvalue(result, 0, 0), PQgetlength(result, 0, 0))) ERROR("postgres_response\n");
}

int postgres_response(request_t *request, enum http_status code, char *body, int length) {
    int error = 0;
    if ((error = !request)) { ERROR("no_request\n"); return error; }
    client_t *client = request->client;
//    DEBUG("result=%p, postgres=%p, request=%p, client=%p\n", result, postgres, request, client);
    if ((error = client->tcp.type != UV_TCP)) { ERROR("client=%p, request=%p, client->tcp.type=%i\n", client, request, client->tcp.type); return error; }
    if ((error = client->tcp.flags > MAX_FLAG)) { ERROR("client=%p, request=%p, client->tcp.flags=%u\n", client, request, client->tcp.flags); return error; }
    if ((error = uv_is_closing((const uv_handle_t *)&client->tcp))) { ERROR("uv_is_closing\n"); return error; } // int uv_is_closing(const uv_handle_t* handle)
    request_free(request);
    if ((error = response_write(client, code, body, length))) { ERROR("response_write\n"); client_close(client); return error; } // char *PQgetvalue(const PGresult *res, int row_number, int column_number); int PQgetlength(const PGresult *res, int row_number, int column_number)
    return error;
}

int postgres_push(postgres_t *postgres) {
//    DEBUG("postgres=%p, postgres->request=%p\n", postgres, postgres->request);
    int error = 0;
    if ((error = postgres_socket(postgres))) { ERROR("postgres_socket\n"); return error; }
    if ((error = PQisBusy(postgres->conn))) { ERROR("PQisBusy\n"); return error; }
    pointer_remove(&postgres->server_pointer);
    postgres->request = NULL;
    server_t *server = (server_t *)postgres->poll.loop->data;
    queue_put_pointer(&server->postgres_queue, &postgres->server_pointer);
    return postgres_process(server);
}

int postgres_pop(postgres_t *postgres) {
//    DEBUG("postgres=%p\n", postgres);
    int error = 0;
    if ((error = postgres_socket(postgres))) { ERROR("postgres_socket\n"); return error; }
    if ((error = PQisBusy(postgres->conn))) { ERROR("PQisBusy\n"); return error; }
    pointer_remove(&postgres->server_pointer);
    return error;
}

int postgres_cancel(postgres_t *postgres) {
//    DEBUG("postgres=%p\n", postgres);
    int error = 0;
    postgres->request = NULL;
    if ((error = postgres_socket(postgres))) { ERROR("postgres_socket\n"); return error; }
    if (!PQisBusy(postgres->conn)) return postgres_push(postgres);
    PGcancel *cancel = PQgetCancel(postgres->conn); // PGcancel *PQgetCancel(PGconn *conn)
    if ((error = !cancel)) { ERROR("PQgetCancel\n"); return error; }
    int errbufsize = 256; char errbuf[errbufsize];
    if ((error = !PQcancel(cancel, errbuf, errbufsize))) ERROR("PQcancel:%s\n", errbuf); // int PQcancel(PGcancel *cancel, char *errbuf, int errbufsize)
    PQfreeCancel(cancel); // void PQfreeCancel(PGcancel *cancel)
//    if ((error = postgres_push(postgres))) { ERROR("postgres_push\n"); return error; }
    return error;
}

int postgres_process(server_t *server) {
//    DEBUG("queue_count(&server->postgres_queue)=%i, queue_count(&server->client_queue)=%i, queue_count(&server->request_queue)=%i\n", queue_count(&server->postgres_queue), queue_count(&server->client_queue), queue_count(&server->request_queue));
    int error = 0;
    if (queue_empty(&server->postgres_queue) || queue_empty(&server->request_queue)) return error;
    request_t *request = pointer_data(queue_get_pointer(&server->request_queue), request_t, server_pointer);
    if ((error = request_pop(request))) { ERROR("request_pop\n"); return error; }
    postgres_t *postgres = pointer_data(queue_get_pointer(&server->postgres_queue), postgres_t, server_pointer);
    postgres->request = request;
    request->postgres = postgres;
    if ((error = postgres_pop(postgres))) { ERROR("postgres_pop\n"); request_free(request); return error; }
//    if ((error = response_write(request, "hi", sizeof("hi") - 1))) { ERROR("response_write\n"); request_close(request->client); } return error; // char *PQgetvalue(const PGresult *res, int row_number, int column_number); int PQgetlength(const PGresult *res, int row_number, int column_number)
//    DEBUG("request=%p, request->client=%p\n", request, request->client);
    if ((error = !PQsendQuery(postgres->conn, "select to_json(now());"))) { ERROR("PQsendQuery:%s", PQerrorMessage(postgres->conn)); request_free(request); return error; } // int PQsendQuery(PGconn *conn, const char *command); char *PQerrorMessage(const PGconn *conn)
    if ((error = uv_poll_start(&postgres->poll, UV_WRITABLE, postgres_on_poll))) { ERROR("uv_poll_start\n"); request_free(request); return error; } // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
    return error;
}

enum http_status sqlstate_code(char *sqlstate) {
    switch (sqlstate[0]) {
        case '0': switch(sqlstate[1]) {
            case '8': return HTTP_STATUS_SERVICE_UNAVAILABLE;
            case 'L': return HTTP_STATUS_FORBIDDEN;
            case 'P': return HTTP_STATUS_FORBIDDEN;
        } break;
        case '2': switch(sqlstate[1]) {
            case '3': switch(sqlstate[2]) {
                case '5': switch(sqlstate[3]) {
                    case '0': switch(sqlstate[4]) {
                        case '3': return HTTP_STATUS_CONFLICT;
                        case '5': return HTTP_STATUS_CONFLICT;
                    } break;
                } break;
            } break;
            case '8': return HTTP_STATUS_FORBIDDEN;
        } break;
        case '4': switch(sqlstate[1]) {
            case '2': switch(sqlstate[2]) {
                case '5': switch(sqlstate[3]) {
                    case '0': switch(sqlstate[4]) {
                        case '1': return HTTP_STATUS_UNAUTHORIZED;
                    } break;
                } break;
                case '8': switch(sqlstate[3]) {
                    case '8': switch(sqlstate[4]) {
                        case '3': return HTTP_STATUS_NOT_FOUND;
                    } break;
                } break;
                case 'P': switch(sqlstate[3]) {
                    case '0': switch(sqlstate[4]) {
                        case '1': return HTTP_STATUS_NOT_FOUND;
                    } break;
                } break;
            } break;
            case '8': return HTTP_STATUS_FORBIDDEN;
        } break;
        case '5': switch(sqlstate[1]) {
            case '3': return HTTP_STATUS_SERVICE_UNAVAILABLE;
            case '4': return HTTP_STATUS_PAYLOAD_TOO_LARGE;
        } break;
        case 'P': switch(sqlstate[1]) {
            case '0': switch(sqlstate[2]) {
                case '0': switch(sqlstate[3]) {
                    case '0': switch(sqlstate[4]) {
                        case '1': return HTTP_STATUS_BAD_REQUEST;
                        case '2': return HTTP_STATUS_NO_RESPONSE;
                    } break;
                } break;
            } break;
            case '8': return HTTP_STATUS_FORBIDDEN;
        } break;
    }
    return HTTP_STATUS_INTERNAL_SERVER_ERROR;
}
