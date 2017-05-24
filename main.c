#include <stdlib.h>
#include <uv.h>
#include "macros.h"
#include "main.h"
#include "request.h"

int main(int argc, char **argv) {
    if (uv_replace_allocator(malloc, realloc, calloc, free)) { // int uv_replace_allocator(uv_malloc_func malloc_func, uv_realloc_func realloc_func, uv_calloc_func calloc_func, uv_free_func free_func)
        ERROR("uv_replace_allocator\n");
        return errno;
    }
    uv_loop_t loop;
    if (uv_loop_init(&loop)) { // int uv_loop_init(uv_loop_t* loop)
        ERROR("uv_loop_init\n");
        return errno;
    }
    uv_tcp_t handle;
    if (uv_tcp_init(&loop, &handle)) { // int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle)
        ERROR("uv_tcp_init\n");
        return errno;
    }
    struct sockaddr_in addr;
#   define IP "0.0.0.0"
#   define PORT 8080
    if (uv_ip4_addr(IP, PORT, &addr)) { // int uv_ip4_addr(const char* ip, int port, struct sockaddr_in* addr)
        ERROR("uv_ip4_addr IP=%s, PORT=%i\n", IP, PORT);
        return errno;
    }
    if (uv_tcp_bind(&handle, (const struct sockaddr *)&addr, 0)) { // int uv_tcp_bind(uv_tcp_t* handle, const struct sockaddr* addr, unsigned int flags)
        ERROR("uv_tcp_bind\n");
        return errno;
    }
#   define THREADS 8
    uv_thread_t tid[THREADS];
    for (int i = 0; i < THREADS; i++) {
        if (uv_thread_create(&tid[i], on_start, (void *)&handle.io_watcher.fd)) { // int uv_thread_create(uv_thread_t* tid, uv_thread_cb entry, void* arg)
            ERROR("uv_thread_create\n");
            return errno;
        }
    }
    for (int i = 0; i < THREADS; i++) {
        if (uv_thread_join(&tid[i])) { // int uv_thread_join(uv_thread_t *tid)
            ERROR("uv_thread_join\n");
            return errno;
        }
    }
    if (uv_run(&loop, UV_RUN_DEFAULT)) { // int uv_run(uv_loop_t* loop, uv_run_mode mode)
        ERROR("uv_run\n");
        return errno;
    }
    return errno;
}
