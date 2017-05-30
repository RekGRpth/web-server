#ifndef _MACROS_H
#define _MACROS_H

#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

//#define DEBUG(fmt, ...) fprintf(stdout, "DEBUG:%lu:%s:%d:%s:" fmt, syscall(SYS_gettid), __FILE__, __LINE__, __func__, ##__VA_ARGS__)
//#define ERROR(fmt, ...) fprintf(stderr, "ERROR:%lu:%s:%d:%s(%i)%s:" fmt, syscall(SYS_gettid), __FILE__, __LINE__, __func__, errno, strerror(errno), ##__VA_ARGS__)
#define DEBUG(fmt, ...) fprintf(stdout, "DEBUG:%lu:%li:%s:%d:%s:" fmt, syscall(SYS_gettid), clock(), __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#define ERROR(fmt, ...) fprintf(stderr, "ERROR:%lu:%li:%s:%d:%s(%i)%s:" fmt, syscall(SYS_gettid), clock(), __FILE__, __LINE__, __func__, errno, strerror(errno), ##__VA_ARGS__)

#endif // _MACROS_H
