#ifndef _MAIN_H
#define _MAIN_H

// from request.c
void request_on_start(void *arg); // void (*uv_thread_cb)(void* arg)

// to main.c
int main(int argc, char **argv);

#endif // _MAIN_H
