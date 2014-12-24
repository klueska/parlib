#include "internal/parlib.h"
#include "internal/syscall.h"

#ifdef __SUPPORTED_C_LIBRARY__

/* From the gnu ld manual for the example of wrappign malloc:
 * You may wish to provide a __real_malloc function as well, so that links
 * without the --wrap option will succeed. If you do this, you should not put
 * the definition of __real_malloc in the same file as __wrap_malloc; if you
 * do, the assembler may resolve the call before the linker has a chance to
 * wrap it to malloc. */

int EXPORT_SYMBOL __real_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  printf("Fatal: __real_accept should be defined using the -Wl,wrap option from ld.");
  exit(1);
}

int EXPORT_SYMBOL __real_socket(int sfamily, int stype, int prot)
{
  printf("Fatal: __real_socket should be defined using the -Wl,wrap option from ld.");
  exit(1);
}

#endif
