#include <stdlib.h>
#include <uv.h>
#include "macros.h"
#include "main.h"

int main(int argc, char **argv) {
    if (uv_replace_allocator(malloc, realloc, calloc, free)) { ERROR("uv_replace_allocator\n"); return errno; } // int uv_replace_allocator(uv_malloc_func malloc_func, uv_realloc_func realloc_func, uv_calloc_func calloc_func, uv_free_func free_func)
    if (postgres_pool_create()) { ERROR("postgres_pool_create\n"); return errno; }
    int cpu_count;
    uv_cpu_info_t *cpu_infos;
    if (uv_cpu_info(&cpu_infos, &cpu_count)) { ERROR("uv_cpu_info\n"); return errno; } // int uv_cpu_info(uv_cpu_info_t** cpu_infos, int* count)
    uv_free_cpu_info(cpu_infos, cpu_count); // void uv_free_cpu_info(uv_cpu_info_t* cpu_infos, int count)
    char *uv_threadpool_size = getenv("UV_THREADPOOL_SIZE"); // char *getenv(const char *name);
    if (!uv_threadpool_size) {
        int length = sizeof("%d") - 1;
        for (int number = cpu_count; number /= 10; length++);
        char str[length];
        if (snprintf(str, length, "%d", cpu_count) != length - 1) { ERROR("snprintf\n"); return errno; } // int snprintf(char *str, size_t size, const char *format, ...)
        if (setenv("UV_THREADPOOL_SIZE", str, 1)) { ERROR("setenv\n"); return errno; } // int setenv(const char *name, const char *value, int overwrite)
    }
    uv_loop_t loop;
    if (uv_loop_init(&loop)) { ERROR("uv_loop_init\n"); return errno; } // int uv_loop_init(uv_loop_t* loop)
    uv_tcp_t handle;
    if (uv_tcp_init(&loop, &handle)) { ERROR("uv_tcp_init\n"); return errno; } // int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle)
    char *ip = getenv("WEBSERVER_IP"); // char *getenv(const char *name);
    if (!ip) ip = "0.0.0.0";
    char *webserver_port = getenv("WEBSERVER_PORT"); // char *getenv(const char *name);
    int port = 8080;
    if (webserver_port) port = atoi(webserver_port);
    struct sockaddr_in addr;
    if (uv_ip4_addr(ip, port, &addr)) { ERROR("uv_ip4_addr %s:%i\n", ip, port); return errno; } // int uv_ip4_addr(const char* ip, int port, struct sockaddr_in* addr)
    if (uv_tcp_bind(&handle, (const struct sockaddr *)&addr, 0)) { ERROR("uv_tcp_bind %s:%i\n", ip, port); return errno; } // int uv_tcp_bind(uv_tcp_t* handle, const struct sockaddr* addr, unsigned int flags)
    uv_os_sock_t sock;
    if (uv_fileno((const uv_handle_t*)&handle, (uv_os_fd_t *)&sock)) { ERROR("uv_fileno\n"); return errno; } // int uv_fileno(const uv_handle_t* handle, uv_os_fd_t* fd)
    int thread_count = cpu_count;
    char *webserver_thread_count = getenv("WEBSERVER_THREAD_COUNT"); // char *getenv(const char *name);
    if (webserver_thread_count) thread_count = atoi(webserver_thread_count);
    if (thread_count < 1) thread_count = cpu_count;
    uv_thread_t tid[thread_count];
    for (int i = 0; i < thread_count; i++) if (uv_thread_create(&tid[i], request_on_start, (void *)&sock)) { ERROR("uv_thread_create\n"); return errno; } // int uv_thread_create(uv_thread_t* tid, uv_thread_cb entry, void* arg)
    for (int i = 0; i < thread_count; i++) if (uv_thread_join(&tid[i])) { ERROR("uv_thread_join\n"); return errno; } // int uv_thread_join(uv_thread_t *tid)
    return errno;
}
