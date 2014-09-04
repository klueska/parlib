#include "internal/ucontext_i.h"

/* void __vcore_reenter(void (*entry_func)(), void *stack_pointer) */

ENTRY(__vcore_reenter)
  pop %eax        /* discard the return address */
  pop %eax        /* obtain entry_func */
  pop %ecx        /* obtain stack_pointer */
  add $-8, %ecx   /* get a two-word margin at the top of the stack */
  mov %ecx, %esp  /* sys_set_stack_pointer */
  jmp %eax        /* jump to entry_func */
PSEUDO_END(__vcore_reenter)
