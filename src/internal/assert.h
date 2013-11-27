#ifndef PARLIB_INTERNAL_ASSERT_H
#define PARLIB_INTERNAL_ASSERT_H

#include <assert.h>

//#define PARLIB_DEBUG
#ifndef PARLIB_DEBUG
# undef assert
# define assert(x) (__builtin_constant_p(x) && (x) == 0 ? __builtin_unreachable() : (x))
#endif

#endif
