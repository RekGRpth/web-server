#ifndef _UTIL_H
#define _UTIL_H

#define UV__F_NONBLOCK 1

int uv__make_socketpair(int fds[2], int flags);
int uv__make_pipe(int fds[2], int flags);
int uv__nonblock(int fd, int set);
int uv__cloexec(int fd, int set);
int uv__close_nocheckstdio(int fd);

#endif // _UTIL_H
