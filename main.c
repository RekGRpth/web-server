#include <stdlib.h>
#include <uv.h>
#include "macros.h"
#include "main.h"

int main(int argc, char **argv) {
    if (uv_replace_allocator(malloc, realloc, calloc, free)) { // int uv_replace_allocator(uv_malloc_func malloc_func, uv_realloc_func realloc_func, uv_calloc_func calloc_func, uv_free_func free_func)
        ERROR("uv_replace_allocator\n");
        return errno;
    }
    int cpu_count;
    uv_cpu_info_t *cpu_infos;
    if (uv_cpu_info(&cpu_infos, &cpu_count)) { // int uv_cpu_info(uv_cpu_info_t** cpu_infos, int* count)
        ERROR("uv_cpu_info\n");
        return errno;
    }
    uv_free_cpu_info(cpu_infos, cpu_count); // void uv_free_cpu_info(uv_cpu_info_t* cpu_infos, int count)
    char *uv_threadpool_size = getenv("UV_THREADPOOL_SIZE"); // char *getenv(const char *name);
    if (!uv_threadpool_size) {
        int length = sizeof("%d") - 1;
//        DEBUG("length=%i\n", length);
        for (int number = cpu_count; number /= 10; length++);
        char str[length];
        if (snprintf(str, length, "%d", cpu_count) != length - 1) { // int snprintf(char *str, size_t size, const char *format, ...)
            ERROR("snprintf\n");
            return errno;
        }
        if (setenv("UV_THREADPOOL_SIZE", str, 1)) { // int setenv(const char *name, const char *value, int overwrite)
            ERROR("setenv\n");
            return errno;
        }
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
    char *webserver_ip = getenv("WEBSERVER_IP"); // char *getenv(const char *name);
    if (!webserver_ip) webserver_ip = "0.0.0.0";
    char *webserver_port = getenv("WEBSERVER_PORT"); // char *getenv(const char *name);
    int port = 8080;
    if (webserver_port) port = atoi(webserver_port);
    struct sockaddr_in addr;
    if (uv_ip4_addr(webserver_ip, port, &addr)) { // int uv_ip4_addr(const char* ip, int port, struct sockaddr_in* addr)
        ERROR("uv_ip4_addr:%s:%i\n", webserver_ip, port);
        return errno;
    }
    if (uv_tcp_bind(&handle, (const struct sockaddr *)&addr, 0)) { // int uv_tcp_bind(uv_tcp_t* handle, const struct sockaddr* addr, unsigned int flags)
        ERROR("uv_tcp_bind:%s:%i\n", webserver_ip, port);
        return errno;
    }
    uv_os_sock_t sock;
    if (uv_fileno((const uv_handle_t*)&handle, (uv_os_fd_t *)&sock)) { // int uv_fileno(const uv_handle_t* handle, uv_os_fd_t* fd)
        ERROR("uv_fileno\n");
        return errno;
    }
    int thread_count = cpu_count;
    char *webserver_thread_count = getenv("WEBSERVER_THREAD_COUNT"); // char *getenv(const char *name);
    if (webserver_thread_count) thread_count = atoi(webserver_thread_count);
    if (thread_count < 1) thread_count = cpu_count;
    uv_thread_t tid[thread_count];
    for (int i = 0; i < thread_count; i++) {
        if (uv_thread_create(&tid[i], request_on_start, (void *)&sock)) { // int uv_thread_create(uv_thread_t* tid, uv_thread_cb entry, void* arg)
            ERROR("uv_thread_create\n");
            return errno;
        }
    }
    for (int i = 0; i < thread_count; i++) {
        if (uv_thread_join(&tid[i])) { // int uv_thread_join(uv_thread_t *tid)
            ERROR("uv_thread_join\n");
            return errno;
        }
    }
    return errno;
}
