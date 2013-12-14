#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdarg.h>
#include "syscall.h"
#include "internal/syscall.h"

handle_event_t ev_handlers[MAX_NR_EVENT];

#ifdef __SUPPORTED_C_LIBRARY__

int open(const char* path, int oflag, ...)
{
  va_list vl;
  va_start(vl, oflag);
  int mode = va_arg(vl, int);
  va_end(vl);

  return uthread_blocking_call(__internal_open, path, oflag, mode);
}

int close(int fd)
{
  return uthread_blocking_call(__internal_close, fd);
}

ssize_t read(int fd, void* buf, size_t sz)
{
  return uthread_blocking_call(__internal_read, fd, buf, sz);
}

ssize_t write(int fd, const void* buf, size_t sz)
{
  return uthread_blocking_call(__internal_write, fd, buf, sz);
}

#endif
