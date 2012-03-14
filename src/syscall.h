/* See COPYING.LESSER for copyright information. */
/* Kevin Klues <klueska@cs.berkeley.edu>	*/

#ifndef __ASYNC_WRAPPERS__
#define __ASYNC_WRAPPERS__

#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

struct syscall;
 
#define malloc(...) __async_simulate_blocking(void *, malloc, __VA_ARGS__)
#define async_malloc(...) async_start(malloc, __VA_ARGS__)
#define async_malloc_detached(callback, ...) \
  async_start_detached(void *, callback, malloc, __VA_ARGS__)

#define open(...) __async_simulate_blocking(int, open, __VA_ARGS__)
#define async_open(...) async_start(open, __VA_ARGS__)
#define async_open_detached(callback, ...) \
  async_start_detached(int, callback, open, __VA_ARGS__)

#define close(...) __async_simulate_blocking(int, close, __VA_ARGS__)
#define async_close(...) async_start(close, __VA_ARGS__)
#define async_close_detached(callback, ...) \
  async_start_detached(int, callback, close, __VA_ARGS__)

#define unlink(...) __async_simulate_blocking(int, unlink, __VA_ARGS__)
#define async_unlink(...) async_start(unlink, __VA_ARGS__)
#define async_unlink_detached(callback, ...) \
  async_start_detached(int, callback, unlink, __VA_ARGS__)

/***************************************************************/
/***************************************************************/
/***************************************************************/
/***************************************************************/

#define __safe_call(__func, ...) \
  ((typeof(__func) *)(__func))(__VA_ARGS__)

// Need to fix this to yield uthread / bring it back up when syscall done
#define __async_simulate_blocking(return_type, __func, ...) \
({ \
  void *result; \
  void *handle = async_start(__func, __VA_ARGS__); \
  async_wait(handle, &result); \
  (return_type)result; \
})

#define async_start(__func, ...) \
({ \
  void *do_##__func(void *arg) { \
    return (void*)(long)__safe_call(__func, __VA_ARGS__); \
  } \
  __async_start((void*)do_##__func, false); \
})

#define async_start_detached(return_type, callback, __func, ...) \
({ \
  void *do_##__func(void *arg) { \
    return_type temp = __safe_call(__func, __VA_ARGS__); \
    callback(temp); \
    return (void*)(long)temp; \
  } \
  __async_start((void*)do_##__func, true); \
})

#define async_wait(handle, result) \
  __async_wait(handle, ((void**)result));

#endif // __ASYNC_WRAPPERS__

