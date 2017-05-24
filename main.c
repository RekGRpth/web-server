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
    int cpu_count;
    uv_cpu_info_t *cpu_infos;
    if (uv_cpu_info(&cpu_infos, &cpu_count)) { // int uv_cpu_info(uv_cpu_info_t** cpu_infos, int* count)
        ERROR("uv_cpu_info\n");
        return errno;
    }
    uv_free_cpu_info(cpu_infos, cpu_count); // void uv_free_cpu_info(uv_cpu_info_t* cpu_infos, int count)
#ifndef REQUEST_THREAD_COUNT
#   define REQUEST_THREAD_COUNT 0
#endif
#if REQUEST_THREAD_COUNT == 0
    const int request_thread_count = cpu_count;
#else
    const int request_thread_count = REQUEST_THREAD_COUNT;
#endif
    uv_thread_t request_tid[request_thread_count];
    for (int i = 0; i < request_thread_count; i++) {
        if (uv_thread_create(&request_tid[i], request_on_start, (void *)&sock)) { // int uv_thread_create(uv_thread_t* tid, uv_thread_cb entry, void* arg)
            ERROR("uv_thread_create\n");
            return errno;
        }
    }
    for (int i = 0; i < request_thread_count; i++) {
        if (uv_thread_join(&request_tid[i])) { // int uv_thread_join(uv_thread_t *tid)
            ERROR("uv_thread_join\n");
            return errno;
        }
    }
    return errno;
}
