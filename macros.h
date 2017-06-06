#ifndef _MACROS_H
#define _MACROS_H

#include <stdio.h> // fprintf, stdout
#include <string.h> // strerror
#include <sys/syscall.h> // SYS_gettid
#include <unistd.h> // syscall

//#define DEBUG(fmt, ...) fprintf(stdout, "DEBUG:%lu:%s:%d:%s:" fmt, syscall(SYS_gettid), __FILE__, __LINE__, __func__, ##__VA_ARGS__)
//#define ERROR(fmt, ...) fprintf(stderr, "ERROR:%lu:%s:%d:%s(%i)%s:" fmt, syscall(SYS_gettid), __FILE__, __LINE__, __func__, errno, strerror(errno), ##__VA_ARGS__)
//#define DEBUG(fmt, ...) fprintf(stdout, "DEBUG:%lu:%li:%s:%d:%s:" fmt, syscall(SYS_gettid), clock(), __FILE__, __LINE__, __func__, ##__VA_ARGS__)
//#define ERROR(fmt, ...) fprintf(stderr, "ERROR:%lu:%li:%s:%d:%s(%i)%s:" fmt, syscall(SYS_gettid), clock(), __FILE__, __LINE__, __func__, errno, strerror(errno), ##__VA_ARGS__)
#define DEBUG(fmt, ...) fprintf(stderr, "DEBUG:%lu:%lu:%s:%d:%s:" fmt, syscall(SYS_gettid), uv_hrtime(), __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#define ERROR(fmt, ...) fprintf(stderr, "ERROR:%lu:%lu:%s:%d:%s(%i)%s:" fmt, syscall(SYS_gettid), uv_hrtime(), __FILE__, __LINE__, __func__, errno, strerror(errno), ##__VA_ARGS__)

#ifndef ARRAY_SIZE
#   define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef ELEM_AT
#   define ELEM_AT(a, i, v) ((unsigned int) (i) < ARRAY_SIZE(a) ? (a)[(i)] : (v))
#endif

#endif // _MACROS_H
