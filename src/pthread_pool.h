#ifndef PARLIB_TPOOL_H
#define PARLIB_TPOOL_H

void pooled_pthread_start(void *(*start_rountine)(void*), void *arg);

#endif
