#include "xbuffer.h" // xbuf_*
#include "macros.h" // DEBUG, ERROR, FATAL
#include <stdarg.h> // va_start, va_end
#include <stdlib.h> // malloc, free

xbuf_t *xbuf_init(xbuf_t *xbuf) {
    xbuf->stream = open_memstream(&xbuf->base, &xbuf->len);
    if (xbuf->stream) return xbuf;
    FATAL("open_memstream\n");
    return NULL;
}

void xbuf_free(xbuf_t *xbuf) {
    if (xbuf->stream && fclose(xbuf->stream)) FATAL("fclose\n");
    free(xbuf->base);
    xbuf->len = 0;
}

xbuf_t *xbuf_new() {
    xbuf_t *xbuf = (xbuf_t *)malloc(sizeof(xbuf_t));
    if (xbuf) return xbuf_init(xbuf);
    FATAL("malloc\n");
    return NULL;
}

void xbuf_del(xbuf_t *xbuf) {
    if (!xbuf) return;
    xbuf_free(xbuf);
    free(xbuf);
}

size_t xbuf_ncat(xbuf_t *xbuf, const char *str, size_t len) {
    if (!(len = fwrite(str, sizeof(char), len, xbuf->stream))) FATAL("fwrite\n");
    if (fflush(xbuf->stream) < 0) FATAL("fflush\n");
//    fflush(xbuf->stream);
    return len;
}

int xbuf_xcat(xbuf_t *xbuf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int len;
    if ((len = vfprintf(xbuf->stream, fmt, args)) < 0) FATAL("vfprintf\n");
    if (fflush(xbuf->stream) < 0) FATAL("fflush\n");
//    fflush(xbuf->stream);
    va_end(args);
    return len;
}

int xbuf_cat(xbuf_t *xbuf, const char *str) {
    int len;
    if ((len = fprintf(xbuf->stream, "%s", str)) < 0) FATAL("fprintf\n");
    if (fflush(xbuf->stream) < 0) FATAL("fflush\n");
//    fflush(xbuf->stream);
    return len;
}
