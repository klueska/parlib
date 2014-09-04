#include "internal/ucontext_i.h"

/* void __vcore_reenter(void (*entry_func)(), void *stack_pointer) */

ENTRY(__vcore_reenter)
  and $-16, %rsi  /* align stack */
  add $-16, %rsi  /* get a two-word buffer at the top of the stack */
  mov %rsi, %rsp  /* sys_set_stack_pointer */
  jmp %rdi        /* jump to entry_func */
PSEUDO_END(__vcore_reenter)
