#include <stdlib.h> // malloc, free, getenv, atoi
#include "postgres.h"
#include "macros.h"

int postgres_queue(uv_loop_t *loop) {
    int error = 0;
//    server_t *server = (server_t *)loop->data;
//    queue_init(&server->postgres_queue);
//    queue_init(&server->request_queue);
    char *conninfo = getenv("WEBSERVER_POSTGRES_CONNINFO"); // char *getenv(const char *name)
    if (!conninfo) conninfo = "postgresql://localhost?application_name=webserver";
    char *webserver_postgres_count = getenv("WEBSERVER_POSTGRES_COUNT"); // char *getenv(const char *name);
    int count = 1;
    if (webserver_postgres_count) count = atoi(webserver_postgres_count);
    if (count < 1) count = 1;
    for (int i = 0; i < count; i++) if (!postgres_init(loop, conninfo)) ERROR("postgres_init\n");
    return error;
}

postgres_t *postgres_init(uv_loop_t *loop, char *conninfo) {
    postgres_t *postgres = (postgres_t *)malloc(sizeof(postgres_t));
    if (!postgres) { ERROR("malloc\n"); return NULL; }
    postgres->conninfo = conninfo;
    queue_init(&postgres->server_queue);
    if (postgres_connect(loop, postgres)) { ERROR("postgres_connect\n"); postgres_free(postgres); return NULL; }
    return postgres;
}

void postgres_free(postgres_t *postgres) {
    queue_remove(&postgres->server_queue);
    free(postgres);
}

int postgres_connect(uv_loop_t *loop, postgres_t *postgres) {
    int error = 0;
    postgres->conn = PQconnectStart(postgres->conninfo); // PGconn *PQconnectStart(const char *conninfo)
    if ((error = !postgres->conn)) { ERROR("PQconnectStart\n"); return error; }
    PQsetErrorVerbosity(postgres->conn, PQERRORS_VERBOSE); // PGVerbosity PQsetErrorVerbosity(PGconn *conn, PGVerbosity verbosity)
    uv_os_sock_t postgres_sock;
    if ((postgres_sock = PQsocket(postgres->conn)) < 0) { ERROR("PQsocket=%i\n", postgres_sock); return postgres_reconnect(postgres); } // int PQsocket(const PGconn *conn)
    if ((error = uv_poll_init_socket(loop, &postgres->poll, postgres_sock))) { ERROR("uv_poll_init_socket\n"); return error; } // int uv_poll_init_socket(uv_loop_t* loop, uv_poll_t* handle, uv_os_sock_t socket)
    postgres->poll.data = (void *)postgres;
    if ((error = uv_poll_start(&postgres->poll, UV_WRITABLE | UV_READABLE, postgres_on_poll))) { ERROR("uv_poll_start\n"); return error; } // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
    return error;
}

int postgres_reconnect(postgres_t *postgres) {
    int error = 0;
    if (postgres->conn) PQfinish(postgres->conn); // void PQfinish(PGconn *conn)
    if (uv_is_active((uv_handle_t *)&postgres->poll)) uv_poll_stop(&postgres->poll); // int uv_is_active(const uv_handle_t* handle); int uv_poll_stop(uv_poll_t* poll)
    if ((error = postgres_pop_postgres(postgres))) { ERROR("postgres_pop_postgres\n"); return error; }
    if ((error = !postgres_timer_init(postgres))) { ERROR("postgres_timer_init\n"); return error; }
    return error;
}

uv_timer_t *postgres_timer_init(postgres_t *postgres) {
    uv_timer_t *timer = (uv_timer_t *)malloc(sizeof(uv_timer_t));
    if (!timer) { ERROR("malloc\n"); return NULL; }
    if (uv_timer_init(postgres->poll.loop, timer)) { ERROR("uv_timer_init\n"); postgres_timer_free(timer); return NULL; } // int uv_timer_init(uv_loop_t* loop, uv_timer_t* handle)
    timer->data = (void *)postgres;
    if (uv_timer_start(timer, postgres_on_timer, 5000, 0)) { ERROR("uv_timer_start\n"); postgres_timer_free(timer); return NULL; } // int uv_timer_start(uv_timer_t* handle, uv_timer_cb cb, uint64_t timeout, uint64_t repeat)
    return timer;
}

void postgres_timer_free(uv_timer_t *timer) {
    free(timer);
}

void postgres_on_timer(uv_timer_t *handle) { // void (*uv_timer_cb)(uv_timer_t* handle)
    postgres_t *postgres = (postgres_t *)handle->data;
    if (postgres_connect(handle->loop, postgres)) ERROR("postgres_connect\n");
    postgres_timer_free(handle);
}

void postgres_on_poll(uv_poll_t *handle, int status, int events) { // void (*uv_poll_cb)(uv_poll_t* handle, int status, int events)
//    DEBUG("handle=%p, status=%i, events=%i\n", handle, status, events);
    if (status) { ERROR("status=%i\n", status); return; }
    postgres_t *postgres = (postgres_t *)handle->data;
    switch (PQstatus(postgres->conn)) { // ConnStatusType PQstatus(const PGconn *conn)
        case CONNECTION_OK: /*DEBUG("CONNECTION_OK\n"); */break;
        case CONNECTION_BAD: ERROR("PQstatus==CONNECTION_BAD %s\n", PQerrorMessage(postgres->conn)); postgres_reconnect(postgres); return; // char *PQerrorMessage(const PGconn *conn)
        default: switch (PQconnectPoll(postgres->conn)) { // PostgresPollingStatusType PQconnectPoll(PGconn *conn)
//            case PGRES_POLLING_READING: DEBUG("PGRES_POLLING_READING\n"); /*if (uv_poll_start(&postgres->poll, UV_READABLE, postgres_on_poll)) ERROR("uv_poll_start\n");*/ return; // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
//            case PGRES_POLLING_WRITING: DEBUG("PGRES_POLLING_WRITING\n"); /*if (uv_poll_start(&postgres->poll, UV_WRITABLE, postgres_on_poll)) ERROR("uv_poll_start\n");*/ return; // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
            case PGRES_POLLING_FAILED: ERROR("PGRES_POLLING_FAILED\n"); postgres_reconnect(postgres); return;
            case PGRES_POLLING_OK: { /*DEBUG("PGRES_POLLING_OK\n");*/
                if (PQstatus(postgres->conn) != CONNECTION_OK) { ERROR("PQstatus!=CONNECTION_OK\n"); postgres_reconnect(postgres); return; } // ConnStatusType PQstatus(const PGconn *conn)
                if (!PQsendQuery(postgres->conn, "listen \"webserver\";")) ERROR("PQsendQuery:%s\n", PQerrorMessage(postgres->conn)); // int PQsendQuery(PGconn *conn, const char *command); char *PQerrorMessage(const PGconn *conn)
            } break;
//            case PGRES_POLLING_ACTIVE: DEBUG("PGRES_POLLING_ACTIVE\n"); return;
            default: return;
        }
    }
    if (!PQisnonblocking(postgres->conn) && PQsetnonblocking(postgres->conn, 1)) ERROR("PQsetnonblocking:%s\n", PQerrorMessage(postgres->conn)); // int PQisnonblocking(const PGconn *conn); int PQsetnonblocking(PGconn *conn, int arg); char *PQerrorMessage(const PGconn *conn)
    if (events & UV_READABLE) {
        if (!PQconsumeInput(postgres->conn)) { // int PQconsumeInput(PGconn *conn);
            ERROR("PQconsumeInput:%s\n", PQerrorMessage(postgres->conn)); // char *PQerrorMessage(const PGconn *conn)
            if (PQsocket(postgres->conn) < 0) { ERROR("PQsocket\n"); postgres_reconnect(postgres); }
            return;
        }
        if (PQisBusy(postgres->conn)) { ERROR("PQisBusy\n"); return; } // int PQisBusy(PGconn *conn)
        for (PGresult *result; (result = PQgetResult(postgres->conn)); PQclear(result)) switch (PQresultStatus(result)) { // PGresult *PQgetResult(PGconn *conn); void PQclear(PGresult *res); ExecStatusType PQresultStatus(const PGresult *res)
            case PGRES_TUPLES_OK: /* DEBUG("PGRES_TUPLES_OK\n"); */postgres_response(result, postgres); break;
            case PGRES_FATAL_ERROR: ERROR("PGRES_FATAL_ERROR:\n%s\n", PQresultErrorMessage(result)); break; // char *PQresultErrorMessage(const PGresult *res)
            default: break;
        }
        for (PGnotify *notify; (notify = PQnotifies(postgres->conn)); PQfreemem(notify)) { // PGnotify *PQnotifies(PGconn *conn); void PQfreemem(void *ptr)
            DEBUG("Asynchronous notification \"%s\" with payload \"%s\" received from server process with PID %d.\n", notify->relname, notify->extra, notify->be_pid);
        }
        server_t *server = (server_t *)postgres->poll.loop->data;
//        DEBUG("queue_empty(&server->postgres_queue)=%i, queue_empty(&server->request_queue)=%i\n", queue_empty(&server->postgres_queue), queue_empty(&server->request_queue));
        DEBUG("queue_count(&server->postgres_queue)=%i, queue_count(&server->request_queue)=%i\n", queue_count(&server->postgres_queue), queue_count(&server->request_queue));
        if (postgres_push_postgres(postgres)) ERROR("postgres_push_postgres\n");
//        server_t *server = (server_t *)postgres->poll.loop->data;
//        DEBUG("queue_empty(&server->postgres_queue)=%i, queue_empty(&server->request_queue)=%i\n", queue_empty(&server->postgres_queue), queue_empty(&server->request_queue));
    }
    if (events & UV_WRITABLE) switch (PQflush(postgres->conn)) { // int PQflush(PGconn *conn);
        case 0: /*DEBUG("No data left to send\n"); */if (uv_poll_start(&postgres->poll, UV_READABLE, postgres_on_poll)) ERROR("uv_poll_start\n"); return; // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
        case 1: /*DEBUG("More data left to send\n"); */return;
        default: ERROR("error sending query\n"); return;
    }
}

void postgres_response(PGresult *result, postgres_t *postgres) {
//    DEBUG("result=%p, postgres=%p\n", result, postgres);
    request_t *request = postgres->request;
    if (!request) { ERROR("no_request\n"); ERROR("postgres=%p\n", postgres); return; }
    if (PQntuples(result) == 0 || PQnfields(result) == 0 || PQgetisnull(result, 0, 0)) { ERROR("no_data_found\n"); request_free(request); return; } // int PQntuples(const PGresult *res); int PQnfields(const PGresult *res); int PQgetisnull(const PGresult *res, int row_number, int column_number)
//    if (!uv_has_ref((const uv_handle_t *)&postgres->request->client->tcp)) { ERROR("uv_has_ref\n"); ERROR("postgres->request=%p\n", postgres->request); return; } // int uv_has_ref(const uv_handle_t* handle)
    client_t *client = request->client;
    if (uv_is_closing((const uv_handle_t *)&client->tcp)) { ERROR("uv_is_closing\n"); ERROR("postgres->request=%p\n", postgres->request); request_free(request); return; } // int uv_is_closing(const uv_handle_t* handle)
    DEBUG("postgres->request=%p\n", postgres->request);
    request_free(request);
    if (response_write(client, PQgetvalue(result, 0, 0), PQgetlength(result, 0, 0))) { ERROR("response_write\n"); request_close(client); } // char *PQgetvalue(const PGresult *res, int row_number, int column_number); int PQgetlength(const PGresult *res, int row_number, int column_number)
}

int postgres_push_postgres(postgres_t *postgres) {
    DEBUG("postgres=%p\n", postgres);
    int error = 0;
    if ((error = PQisBusy(postgres->conn))) { ERROR("PQisBusy\n"); return error; }
    queue_remove(&postgres->server_queue);
    postgres->request = NULL;
    server_t *server = (server_t *)postgres->poll.loop->data;
    queue_insert_tail(&server->postgres_queue, &postgres->server_queue);
    return postgres_process(server);
}

int postgres_pop_postgres(postgres_t *postgres) {
    DEBUG("postgres=%p\n", postgres);
    int error = 0;
    if ((error = PQisBusy(postgres->conn))) { ERROR("PQisBusy\n"); return error; }
    queue_remove(&postgres->server_queue);
    postgres->request = NULL;
    return error;
}

int postgres_push_request(request_t *request) {
    DEBUG("request=%p\n", request);
    int error = 0;
    client_t *client = request->client;
    queue_remove(&request->server_queue);
    if ((error = uv_is_closing((const uv_handle_t *)&client->tcp))) { ERROR("uv_is_closing\n"); return error; } // int uv_is_closing(const uv_handle_t* handle)
    queue_remove(&request->client_queue);
    request->postgres = NULL;
//    int error = 0; if ((error = response_write(request, "hi\n", sizeof("hi\n") - 1))) { ERROR("response_write\n"); request_close(request->client); } return error; // char *PQgetvalue(const PGresult *res, int row_number, int column_number); int PQgetlength(const PGresult *res, int row_number, int column_number)
    server_t *server = (server_t *)client->tcp.loop->data;
    queue_insert_tail(&server->request_queue, &request->server_queue);
    queue_insert_tail(&client->request_queue, &request->client_queue);
    return postgres_process(server);
}

int postgres_pop_request(request_t *request) {
    DEBUG("request=%p\n", request);
    int error = 0;
    client_t *client = request->client;
    queue_remove(&request->server_queue);
    if ((error = uv_is_closing((const uv_handle_t *)&client->tcp))) { ERROR("uv_is_closing\n"); return error; } // int uv_is_closing(const uv_handle_t* handle)
    queue_remove(&request->client_queue);
    request->postgres = NULL;
    return error;
}

int postgres_process(server_t *server) {
//    DEBUG("queue_empty(&server->postgres_queue)=%i, queue_empty(&server->request_queue)=%i\n", queue_empty(&server->postgres_queue), queue_empty(&server->request_queue));
    DEBUG("queue_count(&server->postgres_queue)=%i, queue_count(&server->request_queue)=%i\n", queue_count(&server->postgres_queue), queue_count(&server->request_queue));
    int error = 0;
    if (queue_empty(&server->postgres_queue) || queue_empty(&server->request_queue)) return error;
    queue_t *queue = queue_head(&server->request_queue);
    request_t *request = queue_data(queue, request_t, server_queue);
    if ((error = postgres_pop_request(request))) { ERROR("postgres_pop_request\n"); return error; }
    queue = queue_head(&server->postgres_queue);
    postgres_t *postgres = queue_data(queue, postgres_t, server_queue);
//    if ((error = PQisBusy(postgres->conn))) { ERROR("PQisBusy\n"); return error; }
    if ((error = postgres_pop_postgres(postgres))) { ERROR("postgres_pop_postgres\n"); request_free(request); return error; }
    postgres->request = request;
    request->postgres = postgres;
//    if ((error = response_write(request, "hi", sizeof("hi") - 1))) { ERROR("response_write\n"); request_close(request->client); } return error; // char *PQgetvalue(const PGresult *res, int row_number, int column_number); int PQgetlength(const PGresult *res, int row_number, int column_number)
    if ((error = !PQsendQuery(postgres->conn, "select to_json(now());"))) { ERROR("PQsendQuery:%s\n", PQerrorMessage(postgres->conn)); request_free(request); return error; } // int PQsendQuery(PGconn *conn, const char *command); char *PQerrorMessage(const PGconn *conn)
//    if ((error = uv_poll_start(&postgres->poll, UV_WRITABLE, postgres_on_poll))) { ERROR("uv_poll_start\n"); return error; } // int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
    return error;
}
