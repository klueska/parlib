#include <pthread.h>
#include "parlib.h"
#include "syscall.h"
#include "internal/tls.h"
#include "tls.h"

void *__async_start(void *func, bool detached) {
  pthread_attr_t attr;
  pthread_t *handle = (pthread_t*)__safe_call(malloc, sizeof(pthread_t));
  pthread_attr_init(&attr);
  if(detached)
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
#ifdef __i386__
  int entry, ldt, perms;
  TLS_GET_SEGMENT_REGISTER(&entry, &ldt, &perms);
  TLS_SET_SEGMENT_REGISTER(DEFAULT_TLS_GDT_ENTRY, 0);
#endif
  __safe_call(pthread_create, handle, &attr, (void* (*)(void*))func, NULL);
#ifdef __i386__
  TLS_SET_SEGMENT_REGISTER(entry, ldt);
#endif
  return (void*)handle;
}

int __async_wait(void *handle, void **result) {
  int ret = __safe_call(pthread_join, *((pthread_t*)handle), result);
  __safe_call(free, handle);
  return ret;
}

