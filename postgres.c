#include <stdlib.h>
#include <uv.h>
#include "context.h"
#include "macros.h"
#include "postgres.h"
#include "queue.h"

int postgres_queue(uv_loop_t *loop) {
    server_t *server = (server_t *)loop->data;
    if (uv_cond_init(&server->cond)) { ERROR("uv_cond_init\n"); return errno; } // int uv_cond_init(uv_cond_t* cond)
    if (uv_mutex_init(&server->mutex)) { ERROR("uv_mutex_init\n"); return errno; } // int uv_mutex_init(uv_mutex_t* handle)
    QUEUE_INIT(&server->queue);
    char *postgres_conninfo = getenv("WEBSERVER_POSTGRES_CONNINFO"); // char *getenv(const char *name)
    if (!postgres_conninfo) postgres_conninfo = "postgresql://localhost?application_name=webserver";
    char *webserver_postgres_count = getenv("WEBSERVER_POSTGRES_COUNT"); // char *getenv(const char *name);
    int count = 1;
    if (webserver_postgres_count) count = atoi(webserver_postgres_count);
    if (count < 1) count = 1;
    for (int i = 0; i < count; i++) {
        postgres_t *postgres = (postgres_t *)malloc(sizeof(postgres_t));
        if (!postgres) { ERROR("malloc\n"); continue; }
        postgres->conninfo = postgres_conninfo;
        if (postgres_connect(loop, postgres)) { ERROR("postgres_connect\n"); free(postgres); continue; }
    }
    return 0;
}

int postgres_connect(uv_loop_t *loop, postgres_t *postgres) {
    if (!(postgres->conn = PQconnectStart(postgres->conninfo))) { ERROR("PQconnectStart\n"); return errno; } // PGconn *PQconnectStart(const char *conninfo)
    PQsetErrorVerbosity(postgres->conn, PQERRORS_VERBOSE); // PGVerbosity PQsetErrorVerbosity(PGconn *conn, PGVerbosity verbosity)
    uv_os_sock_t postgres_sock;
    if ((postgres_sock = PQsocket(postgres->conn)) < 0) { ERROR("PQsocket=%i\n", postgres_sock); return postgres_reconnect(postgres); } // int PQsocket(const PGconn *conn)
    if (uv_poll_init_socket(loop, &postgres->poll, postgres_sock)) { ERROR("uv_poll_init_socket\n"); return errno; } // int uv_poll_init_socket(uv_loop_t* loop, uv_poll_t* handle, uv_os_sock_t socket)
    postgres->poll.data = postgres;
    if (uv_poll_start(&postgres->poll, UV_WRITABLE, postgres_on_poll)) { ERROR("uv_poll_start\n"); return errno; } // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
    return 0;
}

int postgres_reconnect(postgres_t *postgres) {
    if (postgres->conn) PQfinish(postgres->conn); // void PQfinish(PGconn *conn)
    if (uv_is_active((uv_handle_t *)&postgres->poll)) uv_poll_stop(&postgres->poll); // int uv_is_active(const uv_handle_t* handle); int uv_poll_stop(uv_poll_t* poll)
    uv_timer_t *timer = (uv_timer_t *)malloc(sizeof(uv_timer_t));
    if (!timer) { ERROR("malloc\n"); return errno; }
    if (uv_timer_init(postgres->poll.loop, timer)) { ERROR("uv_timer_init\n"); free(timer); return errno; } // int uv_timer_init(uv_loop_t* loop, uv_timer_t* handle)
    timer->data = postgres;
    if (uv_timer_start(timer, postgres_on_timer, 5000, 0)) { ERROR("uv_timer_start\n"); free(timer); return errno; } // int uv_timer_start(uv_timer_t* handle, uv_timer_cb cb, uint64_t timeout, uint64_t repeat)
    return 0;
}

void postgres_on_timer(uv_timer_t *handle) { // void (*uv_timer_cb)(uv_timer_t* handle)
    postgres_t *postgres = (postgres_t *)handle->data;
    postgres_connect(handle->loop, postgres);
    free(handle);
}

void postgres_on_poll(uv_poll_t *handle, int status, int events) { // void (*uv_poll_cb)(uv_poll_t* handle, int status, int events)
//    DEBUG("handle=%p, status=%i, events=%i\n", handle, status, events);
    if (status) { ERROR("status=%i\n", status); return; }
    if (events & UV_READABLE) DEBUG("UV_READABLE\n");
    if (events & UV_WRITABLE) DEBUG("UV_WRITABLE\n");
#ifdef UV_DISCONNECT
    if (events & UV_DISCONNECT) { DEBUG("UV_DISCONNECT\n"); postgres_reconnect(postgres); return; }
#endif
    postgres_t *postgres = (postgres_t *)handle->data;
    switch (PQstatus(postgres->conn)) { // ConnStatusType PQstatus(const PGconn *conn)
        case CONNECTION_OK: DEBUG("CONNECTION_OK\n"); break;
        case CONNECTION_BAD: ERROR("PQstatus==CONNECTION_BAD %s\n", PQerrorMessage(postgres->conn)); postgres_reconnect(postgres); return; // char *PQerrorMessage(const PGconn *conn)
        default: switch (PQconnectPoll(postgres->conn)) { // PostgresPollingStatusType PQconnectPoll(PGconn *conn)
            case PGRES_POLLING_READING: DEBUG("PGRES_POLLING_READING\n"); if (uv_poll_start(&postgres->poll, UV_READABLE, postgres_on_poll)) { ERROR("uv_poll_start\n"); return; } return; // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
            case PGRES_POLLING_WRITING: DEBUG("PGRES_POLLING_WRITING\n"); if (uv_poll_start(&postgres->poll, UV_WRITABLE, postgres_on_poll)) { ERROR("uv_poll_start\n"); return; } return; // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
            case PGRES_POLLING_FAILED: ERROR("PGRES_POLLING_FAILED\n"); postgres_reconnect(postgres); return;
            case PGRES_POLLING_OK: { DEBUG("PGRES_POLLING_OK\n");
                if (PQstatus(postgres->conn) != CONNECTION_OK) { ERROR("PQstatus!=CONNECTION_OK\n"); postgres_reconnect(postgres); return; } // ConnStatusType PQstatus(const PGconn *conn)
                postgres_push(postgres);
                if (!PQsendQuery(postgres->conn, "listen \"webserver\";")) ERROR("PQsendQuery:%s\n", PQerrorMessage(postgres->conn)); // int PQsendQuery(PGconn *conn, const char *command); char *PQerrorMessage(const PGconn *conn)
            } break;
            case PGRES_POLLING_ACTIVE: DEBUG("PGRES_POLLING_ACTIVE\n"); return;
        }
    }
    if (!PQisnonblocking(postgres->conn) && PQsetnonblocking(postgres->conn, 1)) ERROR("PQsetnonblocking:%s\n", PQerrorMessage(postgres->conn)); // int PQisnonblocking(const PGconn *conn); int PQsetnonblocking(PGconn *conn, int arg); char *PQerrorMessage(const PGconn *conn)
    if (events & UV_READABLE) {
        if (!PQconsumeInput(postgres->conn)) { // int PQconsumeInput(PGconn *conn);
            ERROR("PQconsumeInput:%s\n", PQerrorMessage(postgres->conn)); // char *PQerrorMessage(const PGconn *conn)
#ifndef UV_DISCONNECT
            uv_os_sock_t postgres_sock;
            if ((postgres_sock = PQsocket(postgres->conn)) < 0) { ERROR("PQsocket=%i\n", postgres_sock); postgres_reconnect(postgres); }
#endif
            return;
        }
        if (PQisBusy(postgres->conn)) return; // int PQisBusy(PGconn *conn)
        for (PGresult *result; (result = PQgetResult(postgres->conn)); PQclear(result)) { // PGresult *PQgetResult(PGconn *conn); void PQclear(PGresult *res)
            switch (PQresultStatus(result)) { // ExecStatusType PQresultStatus(const PGresult *res)
                case PGRES_TUPLES_OK: {
                    DEBUG("PGRES_TUPLES_OK\n"); 
                    if (PQntuples(result) == 0 || PQnfields(result) == 0 || PQgetisnull(result, 0, 0)) ERROR("no_data_found\n"); // int PQntuples(const PGresult *res); int PQnfields(const PGresult *res); int PQgetisnull(const PGresult *res, int row_number, int column_number)
                    if (response_write_response(postgres->client, PQgetvalue(result, 0, 0), PQgetlength(result, 0, 0))) { ERROR("response_write_response\n"); request_close((uv_handle_t *)&postgres->client->tcp); } // char *PQgetvalue(const PGresult *res, int row_number, int column_number); int PQgetlength(const PGresult *res, int row_number, int column_number)
                    postgres_push(postgres);
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
        for (PGnotify *notify; (notify = PQnotifies(postgres->conn)); PQfreemem(notify)) { // PGnotify *PQnotifies(PGconn *conn); void PQfreemem(void *ptr)
            DEBUG("Asynchronous notification \"%s\" with payload \"%s\" received from server process with PID %d.\n", notify->relname, notify->extra, notify->be_pid);
        }

    }
    if (events & UV_WRITABLE) {
        switch (PQflush(postgres->conn)) { // int PQflush(PGconn *conn);
            case 0: DEBUG("No data left to send\n"); if (uv_poll_start(&postgres->poll, UV_READABLE, postgres_on_poll)) { ERROR("uv_poll_start\n"); return; } return; // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
            case 1: DEBUG("More data left to send\n"); return;
            default: ERROR("error sending query.\n"); return;
        }
    }
}

int postgres_query(client_t *client) {
    server_t *server = (server_t *)client->tcp.loop->data;
    DEBUG("client=%p, server=%p\n", client, server);
    postgres_t *postgres = postgres_pop(server);
    postgres->client = client;
    if (!PQsendQuery(postgres->conn, "select to_json(now());")) { ERROR("PQsendQuery:%s\n", PQerrorMessage(postgres->conn)); return errno; } // int PQsendQuery(PGconn *conn, const char *command); char *PQerrorMessage(const PGconn *conn)
    if (uv_poll_start(&postgres->poll, UV_WRITABLE, postgres_on_poll)) { ERROR("uv_poll_start\n"); return errno; } // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)*/
    return 0;
}

void postgres_push(postgres_t *postgres) {
    server_t *server = (server_t *)postgres->poll.loop->data;
    uv_mutex_lock(&server->mutex); // void uv_mutex_lock(uv_mutex_t* handle)
    QUEUE_INSERT_TAIL(&server->queue, &postgres->queue);
    uv_cond_signal(&server->cond); // void uv_cond_signal(uv_cond_t* cond)
    uv_mutex_unlock(&server->mutex); // void uv_mutex_unlock(uv_mutex_t* handle)
}

postgres_t *postgres_pop(server_t *server) {
    uv_mutex_lock(&server->mutex); // void uv_mutex_lock(uv_mutex_t* handle)
    while (QUEUE_EMPTY(&server->queue)) uv_cond_wait(&server->cond, &server->mutex); // void uv_cond_wait(uv_cond_t* cond, uv_mutex_t* mutex)
    QUEUE *queue = QUEUE_HEAD(&server->queue); QUEUE_REMOVE(queue); QUEUE_INIT(queue);
    uv_mutex_unlock(&server->mutex); // void uv_mutex_unlock(uv_mutex_t* handle)
    postgres_t *postgres = QUEUE_DATA(queue, postgres_t, queue);
    return postgres;
}
