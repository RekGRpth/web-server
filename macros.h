#ifndef _MACROS_H
#define _MACROS_H

#include <stdio.h> // fprintf, stdout
#include <string.h> // strerror
#include <sys/syscall.h> // SYS_gettid
#include <unistd.h> // syscall

//#define DEBUG(fmt, ...) fprintf(stdout, "DEBUG:%lu:%s:%d:%s:" fmt, syscall(SYS_gettid), __FILE__, __LINE__, __func__, ##__VA_ARGS__)
//#define ERROR(fmt, ...) fprintf(stderr, "ERROR:%lu:%s:%d:%s(%i)%s:" fmt, syscall(SYS_gettid), __FILE__, __LINE__, __func__, errno, strerror(errno), ##__VA_ARGS__)
#define DEBUG(fmt, ...) fprintf(stdout, "DEBUG:%lu:%li:%s:%d:%s:" fmt, syscall(SYS_gettid), clock(), __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#define ERROR(fmt, ...) fprintf(stderr, "ERROR:%lu:%li:%s:%d:%s(%i)%s:" fmt, syscall(SYS_gettid), clock(), __FILE__, __LINE__, __func__, errno, strerror(errno), ##__VA_ARGS__)

#endif // _MACROS_H
