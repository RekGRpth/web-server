#include <errno.h>
#include <stdarg.h> // va_start, va_end
#include <stdlib.h> // malloc, free
#include "xbuffer.h"
#include "macros.h"

xbuf_t *xbuf_init(xbuf_t *xbuf) {
    xbuf->stream = open_memstream(&xbuf->base, &xbuf->len);
    if (xbuf->stream) return xbuf;
    ERROR("open_memstream\n");
    return NULL;
}

void xbuf_free(xbuf_t *xbuf) {
    if (xbuf->stream && fclose(xbuf->stream)) ERROR("fclose\n");
    free(xbuf->base);
    xbuf->len = 0;
}

xbuf_t *xbuf_new() {
    xbuf_t *xbuf = (xbuf_t *)malloc(sizeof(xbuf_t));
    if (xbuf) return xbuf_init(xbuf);
    ERROR("malloc\n");
    return NULL;
}

void xbuf_del(xbuf_t *xbuf) {
    if (!xbuf) return;
    xbuf_free(xbuf);
    free(xbuf);
}

size_t xbuf_ncat(xbuf_t *xbuf, char *str, size_t len) {
    if ((len = fwrite(str, sizeof(char), len, xbuf->stream)) <= 0) ERROR("fwrite\n");
    if (fflush(xbuf->stream) <= 0) ERROR("fflush\n");
    return len;
}

int xbuf_xcat(xbuf_t *xbuf, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int len;
    if ((len = vfprintf(xbuf->stream, fmt, args)) <= 0) ERROR("vfprintf\n");
    if (fflush(xbuf->stream) <= 0) ERROR("fflush\n");
    va_end(args);
    return len;
}

int xbuf_cat(xbuf_t *xbuf, char *str) {
    int len;
    if ((len = fprintf(xbuf->stream, "%s", str)) <= 0) ERROR("fprintf\n");
    if (fflush(xbuf->stream) <= 0) ERROR("fflush\n");
    return len;
}
