#ifndef PARLIB_INTERNAL_ASSERT_H
#define PARLIB_INTERNAL_ASSERT_H

#ifndef __ASSEMBLER__

#include <assert.h>

//#define PARLIB_DEBUG
#ifndef PARLIB_DEBUG
# undef assert
# define assert(x) (__builtin_constant_p(x) && (x) == 0 ? __builtin_unreachable() : (x))
#else
# undef assert
# define assert(x) (x)
#endif

#endif // __ASSEMBLER__

#define INTERNAL(name) plt_bypass_ ## name

#endif // PARLIB_INTERNAL_ASSERT_H
