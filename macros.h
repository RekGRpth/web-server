#ifndef _MACROS_H
#define _MACROS_H

#include <stdio.h>

#define DEBUG(fmt, ...)  fprintf(stdout, "DEBUG:%s:%d:%s:" fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#define ERROR(fmt, ...)  fprintf(stderr, "ERROR:%s:%d:%s(%i)%s:" fmt, __FILE__, __LINE__, __func__, errno, strerror(errno), ##__VA_ARGS__)

#endif // _MACROS_H
