#include <postgresql/libpq-fe.h>
#include <stdlib.h>
#include <uv.h>
#include "context.h"
#include "macros.h"
#include "postgres.h"
#include "queue.h"

int postgres_queue(uv_loop_t *loop) {
    server_t *server = (server_t *)loop->data;
    queue_init(&server->postgres);
    queue_init(&server->client);
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
        postgres->client = NULL;
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
    queue_remove(&postgres->queue);
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
    postgres_t *postgres = (postgres_t *)handle->data;
    switch (PQstatus(postgres->conn)) { // ConnStatusType PQstatus(const PGconn *conn)
        case CONNECTION_OK: /*DEBUG("CONNECTION_OK\n"); */break;
        case CONNECTION_BAD: ERROR("PQstatus==CONNECTION_BAD %s\n", PQerrorMessage(postgres->conn)); postgres_reconnect(postgres); return; // char *PQerrorMessage(const PGconn *conn)
        default: switch (PQconnectPoll(postgres->conn)) { // PostgresPollingStatusType PQconnectPoll(PGconn *conn)
            case PGRES_POLLING_READING: /*DEBUG("PGRES_POLLING_READING\n"); */if (uv_poll_start(&postgres->poll, UV_READABLE, postgres_on_poll)) ERROR("uv_poll_start\n"); return; // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
            case PGRES_POLLING_WRITING: /*DEBUG("PGRES_POLLING_WRITING\n"); */if (uv_poll_start(&postgres->poll, UV_WRITABLE, postgres_on_poll)) ERROR("uv_poll_start\n"); return; // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
            case PGRES_POLLING_FAILED: ERROR("PGRES_POLLING_FAILED\n"); postgres_reconnect(postgres); return;
            case PGRES_POLLING_OK: {/* DEBUG("PGRES_POLLING_OK\n");*/
                if (PQstatus(postgres->conn) != CONNECTION_OK) { ERROR("PQstatus!=CONNECTION_OK\n"); postgres_reconnect(postgres); return; } // ConnStatusType PQstatus(const PGconn *conn)
                if (!PQsendQuery(postgres->conn, "listen \"webserver\";")) ERROR("PQsendQuery:%s\n", PQerrorMessage(postgres->conn)); // int PQsendQuery(PGconn *conn, const char *command); char *PQerrorMessage(const PGconn *conn)
                if (postgres_postgres(postgres)) ERROR("postgres_postgres\n");
            } break;
            case PGRES_POLLING_ACTIVE: /*DEBUG("PGRES_POLLING_ACTIVE\n"); */return;
        }
    }
    if (!PQisnonblocking(postgres->conn) && PQsetnonblocking(postgres->conn, 1)) ERROR("PQsetnonblocking:%s\n", PQerrorMessage(postgres->conn)); // int PQisnonblocking(const PGconn *conn); int PQsetnonblocking(PGconn *conn, int arg); char *PQerrorMessage(const PGconn *conn)
    if (events & UV_READABLE) {
        if (!PQconsumeInput(postgres->conn)) { // int PQconsumeInput(PGconn *conn);
            ERROR("PQconsumeInput:%s\n", PQerrorMessage(postgres->conn)); // char *PQerrorMessage(const PGconn *conn)
            if (PQsocket(postgres->conn) < 0) { ERROR("PQsocket\n"); postgres_reconnect(postgres); }
            return;
        }
        if (PQisBusy(postgres->conn)) return; // int PQisBusy(PGconn *conn)
        for (PGresult *result; (result = PQgetResult(postgres->conn)); PQclear(result)) switch (PQresultStatus(result)) { // PGresult *PQgetResult(PGconn *conn); void PQclear(PGresult *res); ExecStatusType PQresultStatus(const PGresult *res)
            case PGRES_TUPLES_OK: /* DEBUG("PGRES_TUPLES_OK\n"); */postgres_response(result, postgres->client); break;
            case PGRES_FATAL_ERROR: ERROR("PGRES_FATAL_ERROR:\n%s\n", PQresultErrorMessage(result)); break; // char *PQresultErrorMessage(const PGresult *res)
            default: break;
        }
        for (PGnotify *notify; (notify = PQnotifies(postgres->conn)); PQfreemem(notify)) { // PGnotify *PQnotifies(PGconn *conn); void PQfreemem(void *ptr)
            DEBUG("Asynchronous notification \"%s\" with payload \"%s\" received from server process with PID %d.\n", notify->relname, notify->extra, notify->be_pid);
        }
        if (postgres_postgres(postgres)) ERROR("postgres_postgres\n");
    }
    if (events & UV_WRITABLE) switch (PQflush(postgres->conn)) { // int PQflush(PGconn *conn);
        case 0: /*DEBUG("No data left to send\n"); */if (uv_poll_start(&postgres->poll, UV_READABLE, postgres_on_poll)) ERROR("uv_poll_start\n"); return; // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
        case 1: /*DEBUG("More data left to send\n"); */return;
        default: ERROR("error sending query\n"); return;
    }
}

void postgres_response(PGresult *result, client_t *client) {
    if (PQntuples(result) == 0 || PQnfields(result) == 0 || PQgetisnull(result, 0, 0)) { ERROR("no_data_found\n"); return; } // int PQntuples(const PGresult *res); int PQnfields(const PGresult *res); int PQgetisnull(const PGresult *res, int row_number, int column_number)
    if (response_write(client, PQgetvalue(result, 0, 0), PQgetlength(result, 0, 0))) { ERROR("response_write\n"); request_close((uv_handle_t *)&client->tcp); } // char *PQgetvalue(const PGresult *res, int row_number, int column_number); int PQgetlength(const PGresult *res, int row_number, int column_number)
}

int postgres_postgres(postgres_t *postgres) {
    server_t *server = (server_t *)postgres->poll.loop->data;
    queue_insert_tail(&server->postgres, &postgres->queue);
    return postgres_process(server);
}

int postgres_client(client_t *client) {
    server_t *server = (server_t *)client->tcp.loop->data;
    queue_insert_tail(&server->client, &client->queue);
    return postgres_process(server);
}

int postgres_process(server_t *server) {
    if (queue_empty(&server->postgres) || queue_empty(&server->client)) return 0;
    queue_t *queue = queue_head(&server->postgres);
    postgres_t *postgres = queue_data(queue, postgres_t, queue);
    if (PQisBusy(postgres->conn)) return 0;
    queue_remove(queue);
    queue = queue_head(&server->client);
    client_t *client = queue_data(queue, client_t, queue);
    queue_remove(queue);
    postgres->client = client;
    client->postgres = postgres;
    if (!PQsendQuery(postgres->conn, "select to_json(now());")) { ERROR("PQsendQuery:%s\n", PQerrorMessage(postgres->conn)); return errno; } // int PQsendQuery(PGconn *conn, const char *command); char *PQerrorMessage(const PGconn *conn)
    if (uv_poll_start(&postgres->poll, UV_WRITABLE, postgres_on_poll)) { ERROR("uv_poll_start\n"); return errno; } // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
    return 0;
}
