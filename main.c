#include <stdlib.h> // malloc, realloc, calloc, free, getenv, setenv, atoi, size_t
#include <netinet/in.h>  // sockaddr_in
#include <uv.h> // uv_*
#include "main.h"
#include "macros.h" // DEBUG, ERROR
#include "server.h"

int main(int argc, char **argv) {
    int error = 0;
    if ((error = uv_replace_allocator(malloc, realloc, calloc, free))) { FATAL("uv_replace_allocator\n"); return error; } // int uv_replace_allocator(uv_malloc_func malloc_func, uv_realloc_func realloc_func, uv_calloc_func calloc_func, uv_free_func free_func)
    int cpu_count; uv_cpu_info_t *cpu_infos;
    if ((error = uv_cpu_info(&cpu_infos, &cpu_count))) { FATAL("uv_cpu_info\n"); return error; } // int uv_cpu_info(uv_cpu_info_t** cpu_infos, int* count)
    uv_free_cpu_info(cpu_infos, cpu_count); // void uv_free_cpu_info(uv_cpu_info_t* cpu_infos, int count)
    char *uv_threadpool_size = getenv("UV_THREADPOOL_SIZE"); // char *getenv(const char *name);
    if (!uv_threadpool_size) {
        int length = sizeof("%d") - 1;
        for (int number = cpu_count; number /= 10; length++);
        char str[length];
        if ((error = snprintf(str, length, "%d", cpu_count) - length + 1)) { FATAL("snprintf\n"); return error; } // int snprintf(char *str, size_t size, const char *format, ...)
        if ((error = setenv("UV_THREADPOOL_SIZE", str, 1))) { FATAL("setenv\n"); return error; } // int setenv(const char *name, const char *value, int overwrite)
    }
    uv_loop_t loop;
    if ((error = uv_loop_init(&loop))) { FATAL("uv_loop_init\n"); return error; } // int uv_loop_init(uv_loop_t* loop)
    uv_tcp_t tcp;
    if ((error = uv_tcp_init(&loop, &tcp))) { FATAL("uv_tcp_init\n"); return error; } // int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle)
    char *webserver_port = getenv("WEBSERVER_PORT"); // char *getenv(const char *name);
    int port = 8080;
    if (webserver_port) port = atoi(webserver_port);
    struct sockaddr_in addr;
    const char *ip = "0.0.0.0";
    if ((error = uv_ip4_addr(ip, port, &addr))) { FATAL("uv_ip4_addr(%s:%i)\n", ip, port); return error; } // int uv_ip4_addr(const char* ip, int port, struct sockaddr_in* addr)
    if ((error = uv_tcp_bind(&tcp, (const struct sockaddr *)&addr, 0))) { FATAL("uv_tcp_bind(%s:%i)\n", ip, port); return error; } // int uv_tcp_bind(uv_tcp_t* handle, const struct sockaddr* addr, unsigned int flags)
    uv_os_sock_t sock;
    if ((error = uv_fileno((const uv_handle_t*)&tcp, (uv_os_fd_t *)&sock))) { FATAL("uv_fileno\n"); return error; } // int uv_fileno(const uv_handle_t* handle, uv_os_fd_t* fd)
    int thread_count = cpu_count;
    char *webserver_thread_count = getenv("WEBSERVER_THREAD_COUNT"); // char *getenv(const char *name);
    if (webserver_thread_count) thread_count = atoi(webserver_thread_count);
    if (thread_count < 1) thread_count = cpu_count;
    if (thread_count == 1) server_on_start((void *)&sock);
    else {
        uv_thread_t tid[thread_count];
        for (int i = 0; i < thread_count; i++) if ((error = uv_thread_create(&tid[i], server_on_start, (void *)&sock))) { FATAL("uv_thread_create\n"); return error; } // int uv_thread_create(uv_thread_t* tid, uv_thread_cb entry, void* arg)
        for (int i = 0; i < thread_count; i++) if ((error = uv_thread_join(&tid[i]))) { FATAL("uv_thread_join\n"); return error; } // int uv_thread_join(uv_thread_t *tid)
    }
    if ((error = uv_loop_close(&loop))) { FATAL("uv_loop_close\n"); return error; } // int uv_loop_close(uv_loop_t* loop)
    return error;
}
