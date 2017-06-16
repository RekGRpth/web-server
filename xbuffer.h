#ifndef _XBUFFER_H
#define _XBUFFER_H

typedef struct xbuf_s xbuf_t;

#include <stdio.h> // FILE

struct xbuf_s {
    char *base;
    size_t len;
    FILE *stream;
};

xbuf_t *xbuf_init(xbuf_t *xbuf);
void xbuf_free(xbuf_t *xbuf);
xbuf_t *xbuf_new();
void xbuf_del(xbuf_t *xbuf);
size_t xbuf_ncat(xbuf_t *xbuf, const char *str, size_t len);
int xbuf_xcat(xbuf_t *xbuf, const char *fmt, ...);
int xbuf_cat(xbuf_t *xbuf, const char *str);

#endif // _XBUFFER_H
