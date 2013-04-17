#ifndef PARLIB_INTERNAL_ASSERT_H
#define PARLIB_INTERNAL_ASSERT_H

#include <assert.h>

//#define PARLIB_DEBUG
#ifndef PARLIB_DEBUG
# undef assert
# define assert(x) (x)
#endif

#endif
