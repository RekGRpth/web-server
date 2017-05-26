#include <uv.h>
#include "macros.h"
#include "context.h"
#include "postgres.h"

client_t *global_client = NULL;

void postgres_on_poll(uv_poll_t *handle, int status, int events) { // void (*uv_poll_cb)(uv_poll_t* handle, int status, int events)
//    DEBUG("handle=%p, status=%i, events=%i\n", handle, status, events);
    if (events & UV_READABLE) DEBUG("UV_READABLE\n");
    if (events & UV_WRITABLE) DEBUG("UV_WRITABLE\n");
//    if (events & UV_DISCONNECT) DEBUG("UV_DISCONNECT\n");
    if (status) {
        ERROR("status");
        return;
    }
    server_t *server = (server_t *)handle->data;
    switch (PQstatus(server->conn)) { // ConnStatusType PQstatus(const PGconn *conn)
        case CONNECTION_OK: DEBUG("CONNECTION_OK\n"); break;
        case CONNECTION_BAD: {
            ERROR("PQstatus==CONNECTION_BAD %s\n", PQerrorMessage(server->conn)); // char *PQerrorMessage(const PGconn *conn)
            postgres_reconnect(handle->loop);
        } return;
        default: switch (PQconnectPoll(server->conn)) { // PostgresPollingStatusType PQconnectPoll(PGconn *conn)
            case PGRES_POLLING_READING: DEBUG("PGRES_POLLING_READING\n"); if (uv_poll_start(&server->poll, UV_READABLE, postgres_on_poll)) { // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
                ERROR("uv_poll_start\n");
                return;
            } return;
            case PGRES_POLLING_WRITING: DEBUG("PGRES_POLLING_WRITING\n"); if (uv_poll_start(&server->poll, UV_WRITABLE, postgres_on_poll)) { // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
                ERROR("uv_poll_start\n");
                return;
            } return;
            case PGRES_POLLING_FAILED: {
                ERROR("PGRES_POLLING_FAILED\n");
                postgres_reconnect(handle->loop);
            } return;
            case PGRES_POLLING_OK: DEBUG("PGRES_POLLING_OK\n"); { 
                if (PQstatus(server->conn) != CONNECTION_OK) { // ConnStatusType PQstatus(const PGconn *conn)
                    ERROR("PQstatus!=CONNECTION_OK\n");
                    postgres_reconnect(handle->loop);
                    return;
                }
                if (!PQsendQuery(server->conn, "listen \"gwan\";")) { // int PQsendQuery(PGconn *conn, const char *command)
                    ERROR("PQsendQuery:%s\n", PQerrorMessage(server->conn)); // char *PQerrorMessage(const PGconn *conn)
                }
            } break;
            case PGRES_POLLING_ACTIVE: DEBUG("PGRES_POLLING_ACTIVE\n"); return;
        }
    }
    if (!PQisnonblocking(server->conn)) { // int PQisnonblocking(const PGconn *conn);
        if (PQsetnonblocking(server->conn, 1)) { // int PQsetnonblocking(PGconn *conn, int arg)
            ERROR("PQsetnonblocking:%s\n", PQerrorMessage(server->conn)); // char *PQerrorMessage(const PGconn *conn)
        }
    }
    if (events & UV_READABLE) {
        if (!PQconsumeInput(server->conn)) { // int PQconsumeInput(PGconn *conn);
            ERROR("PQconsumeInput:%s\n", PQerrorMessage(server->conn)); // char *PQerrorMessage(const PGconn *conn)
            uv_os_sock_t server_sock;
            if ((server_sock = PQsocket(server->conn)) < 0) { // int PQsocket(const PGconn *conn)
                ERROR("PQsocket=%i\n", server_sock);
                postgres_reconnect(handle->loop);
                return;
            }
            return;
        }
        if (PQisBusy(server->conn)) { // int PQisBusy(PGconn *conn)
            return;
        }
        for (PGresult *result; (result = PQgetResult(server->conn)); PQclear(result)) { // PGresult *PQgetResult(PGconn *conn); void PQclear(PGresult *res)
            switch (PQresultStatus(result)) { // ExecStatusType PQresultStatus(const PGresult *res)
                case PGRES_TUPLES_OK: {
                    DEBUG("PGRES_TUPLES_OK\n"); 
                    if (PQntuples(result) == 0 || PQnfields(result) == 0 || PQgetisnull(result, 0, 0)) { // int PQntuples(const PGresult *res); int PQnfields(const PGresult *res); int PQgetisnull(const PGresult *res, int row_number, int column_number)
                        ERROR("no_data_found\n");
                    }
                    char *response = PQgetvalue(result, 0, 0); // char *PQgetvalue(const PGresult *res, int row_number, int column_number)
                    int length = PQgetlength(result, 0, 0); // int PQgetlength(const PGresult *res, int row_number, int column_number);
                    if (response_write_response(global_client, response, length)) {
                        ERROR("response_write\n");
                        request_close((uv_handle_t *)&global_client->tcp);
                    }
                } break;
                case PGRES_FATAL_ERROR: ERROR("PGRES_FATAL_ERROR:\n%s\n", PQresultErrorMessage(result)); break; // char *PQresultErrorMessage(const PGresult *res)
                case PGRES_COMMAND_OK: DEBUG("PGRES_COMMAND_OK\n"); break;
                case PGRES_COPY_OUT: DEBUG("PGRES_COPY_OUT\n"); break;
                case PGRES_COPY_IN: DEBUG("PGRES_COPY_IN\n"); break;
                case PGRES_BAD_RESPONSE: DEBUG("PGRES_BAD_RESPONSE\n"); break;
                case PGRES_NONFATAL_ERROR: DEBUG("PGRES_NONFATAL_ERROR\n"); break;
                case PGRES_COPY_BOTH: DEBUG("PGRES_COPY_BOTH\n"); break;
                case PGRES_SINGLE_TUPLE: DEBUG("PGRES_SINGLE_TUPLE\n"); break;
                case PGRES_EMPTY_QUERY: DEBUG("PGRES_EMPTY_QUERY\n"); break;
            }
        }
        for (PGnotify *notify; (notify = PQnotifies(server->conn)); PQfreemem(notify)) { // PGnotify *PQnotifies(PGconn *conn); void PQfreemem(void *ptr)
            DEBUG("Asynchronous notification \"%s\" with payload \"%s\" received from server process with PID %d.\n", notify->relname, notify->extra, notify->be_pid);
        }

    }
    if (events & UV_WRITABLE) {
        switch (PQflush(server->conn)) { // int PQflush(PGconn *conn);
            case 0: DEBUG("No data left to send\n"); if (uv_poll_start(&server->poll, UV_READABLE, postgres_on_poll)) { // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
                ERROR("uv_poll_start\n");
                return;
            } return;
            case 1: DEBUG("More data left to send\n"); return;
            default: ERROR("error sending query.\n"); return;
        }
    }
}

int postgres_reconnect(uv_loop_t *loop) {
    server_t *server = (server_t *)loop->data;
    if (server->conn) {
        PQfinish(server->conn); // void PQfinish(PGconn *conn)
    }
    if (uv_is_active((uv_handle_t *)&server->poll)) { // int uv_is_active(const uv_handle_t* handle)
        uv_poll_stop(&server->poll); // int uv_poll_stop(uv_poll_t* poll)
    }
    if (uv_timer_init(loop, &server->timer)) { // int uv_timer_init(uv_loop_t* loop, uv_timer_t* handle)
        ERROR("uv_timer_init\n");
        return errno;
    }
    server->timer.data = server;
    if (uv_timer_start(&server->timer, postgres_on_timer, 5000, 0)) { // int uv_timer_start(uv_timer_t* handle, uv_timer_cb cb, uint64_t timeout, uint64_t repeat)
        ERROR("uv_timer_start\n");
        return errno;
    }
    return errno;
}

void postgres_on_timer(uv_timer_t *handle) { // void (*uv_timer_cb)(uv_timer_t* handle)
    postgres_connect(handle->loop);
}

int postgres_connect(uv_loop_t *loop) {
    server_t *server = (server_t *)loop->data;
    if (!(server->conn = PQconnectStart(server->conninfo))) { // PGconn *PQconnectStart(const char *conninfo)
        ERROR("PQconnectStart\n");
        return errno;
    }
    PQsetErrorVerbosity(server->conn, PQERRORS_VERBOSE); // PGVerbosity PQsetErrorVerbosity(PGconn *conn, PGVerbosity verbosity)
    uv_os_sock_t server_sock;
    if ((server_sock = PQsocket(server->conn)) < 0) { // int PQsocket(const PGconn *conn)
        ERROR("PQsocket=%i\n", server_sock);
        return postgres_reconnect(loop);
    }
    if (uv_poll_init_socket(loop, &server->poll, server_sock)) { // int uv_poll_init_socket(uv_loop_t* loop, uv_poll_t* handle, uv_os_sock_t socket)
        ERROR("uv_poll_init_socket\n");
        return errno;
    }
    server->poll.data = server;
    postgres_on_poll(&server->poll, 0, 0);
    return 0;
}

void postgres_query(client_t *client) {
    global_client = client;
    server_t *server = (server_t *)client->tcp.loop->data;
//    if (!PQsendQuery(server->conn, "select pg_notify('gwan', now()::text);")) { // int PQsendQuery(PGconn *conn, const char *command)
    if (!PQsendQuery(server->conn, "select to_json(now());")) { // int PQsendQuery(PGconn *conn, const char *command)
        ERROR("PQsendQuery:%s\n", PQerrorMessage(server->conn)); // char *PQerrorMessage(const PGconn *conn)
        return;
    }
    if (uv_poll_start(&server->poll, UV_WRITABLE, postgres_on_poll)) { // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
        ERROR("uv_poll_start\n");
        return;
    }
/*    const char *command = "SELECT \"fias\".\"web\".\"route\"($1);";
    const Oid paramTypes[] = {JSONOID};
    int nParams = sizeof(paramTypes) / sizeof(paramTypes[0]);
    const char * const paramValues[] = {"{}"};
    if (!PQsendQueryParams(server->conn, command, nParams, paramTypes, paramValues, NULL, NULL, 0)) { // int PQsendQueryParams(PGconn *conn, const char *command, int nParams, const Oid *paramTypes, const char * const *paramValues, const int *paramLengths, const int *paramFormats, int resultFormat)
        ERROR("PQsendQueryParams:%s\n", PQerrorMessage(server->conn)); // char *PQerrorMessage(const PGconn *conn)
    }*/
}
