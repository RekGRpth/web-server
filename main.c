#include <stdlib.h>
#include <uv.h>
#include "macros.h"
#include "main.h"

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
    uv_os_sock_t sock = handle.io_watcher.fd;
#ifndef REQUEST_THREAD_COUNT
#   define REQUEST_THREAD_COUNT 0
#endif
    int count = REQUEST_THREAD_COUNT;
    switch(count) {
        case 1: {
            request_on_start((void *)&sock);
            return errno;
        } break;
        case 0: {
            uv_cpu_info_t *cpu_infos;
            if (uv_cpu_info(&cpu_infos, &count)) { // int uv_cpu_info(uv_cpu_info_t** cpu_infos, int* count)
                ERROR("uv_cpu_info\n");
                return errno;
            }
            uv_free_cpu_info(cpu_infos, count); // void uv_free_cpu_info(uv_cpu_info_t* cpu_infos, int count)
        } break;
    }
    uv_thread_t request_tid[count];
    for (int i = 0; i < count; i++) {
        if (uv_thread_create(&request_tid[i], request_on_start, (void *)&sock)) { // int uv_thread_create(uv_thread_t* tid, uv_thread_cb entry, void* arg)
            ERROR("uv_thread_create\n");
            return errno;
        }
    }
    for (int i = 0; i < count; i++) {
        if (uv_thread_join(&request_tid[i])) { // int uv_thread_join(uv_thread_t *tid)
            ERROR("uv_thread_join\n");
            return errno;
        }
    }
/*    if (uv_run(&loop, UV_RUN_DEFAULT)) { // int uv_run(uv_loop_t* loop, uv_run_mode mode)
        ERROR("uv_run\n");
        return errno;
    }*/
    return errno;
}
