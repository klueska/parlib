#include "internal/parlib.h"
#include "internal/syscall.h"
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

handle_event_t EXPORT_SYMBOL ev_handlers[MAX_NR_EVENT];

#ifdef __SUPPORTED_C_LIBRARY__

int EXPORT_SYMBOL open(const char* path, int oflag, ...)
{
  va_list vl;
  va_start(vl, oflag);
  int mode = va_arg(vl, int);
  va_end(vl);

  if (current_uthread)
    oflag |= O_NONBLOCK;
  return __internal_open(path, oflag, mode);
}

FILE EXPORT_SYMBOL *fopen(const char *path, const char *mode)
{
  FILE *stream = __internal_fopen(path, mode);
  int fd = fileno(stream);
  int fl = fcntl(fd, F_GETFL, 0);
  fl |= O_NONBLOCK;
  fcntl(fd, F_SETFL, fl);
  return stream;
}

ssize_t EXPORT_SYMBOL read(int fd, void* buf, size_t sz)
{
  ssize_t __blocking_read(int __fd, void *__buf, size_t __sz) {
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(__fd, &fdset);
    select(__fd+1, &fdset, NULL, &fdset, NULL);
    return __internal_read(__fd, __buf, __sz);
  }

  if (current_uthread)
    return uthread_blocking_call(__internal_read, __blocking_read, fd, buf, sz);
  return __internal_read(fd, buf, sz);
}

ssize_t EXPORT_SYMBOL write(int fd, const void* buf, size_t sz)
{
  ssize_t __blocking_write(int __fd, const void *__buf, size_t __sz) {
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(__fd, &fdset);
    select(__fd+1, NULL, &fdset, &fdset, NULL);
    return __internal_write(__fd, __buf, __sz);
  }

  if (current_uthread)
    return uthread_blocking_call(__internal_write, __blocking_write,
                                 fd, buf, sz);
  return __internal_write(fd, buf, sz);
}

size_t EXPORT_SYMBOL fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  ssize_t __blocking_fread(void *__ptr, size_t __size,
                           size_t __nmemb, FILE *__stream)
  {
    int __fd = fileno(__stream);
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(__fd, &fdset);
    select(__fd+1, &fdset, NULL, &fdset, NULL);
    return __internal_fread(__ptr, __size, __nmemb, __stream);
  }

  if (current_uthread)
    return uthread_blocking_call(__internal_fread, __blocking_fread,
                                 ptr, size, nmemb, stream);
  return __internal_fread(ptr, size, nmemb, stream);
}

size_t EXPORT_SYMBOL fwrite(const void *ptr, size_t size,
                            size_t nmemb, FILE *stream)
{
  ssize_t __blocking_fwrite(const void *__ptr, size_t __size,
                            size_t __nmemb, FILE *__stream)
  {
    int __fd = fileno(__stream);
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(__fd, &fdset);
    select(__fd+1, NULL, &fdset, &fdset, NULL);
    return __internal_fwrite(__ptr, __size, __nmemb, __stream);
  }

  if (current_uthread)
    return uthread_blocking_call(__internal_fwrite, __blocking_fwrite,
                                 ptr, size, nmemb, stream);
  return __internal_fwrite(ptr, size, nmemb, stream);
}

#endif
