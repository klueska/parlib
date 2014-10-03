#include "internal/ucontext_i.h"

/* void __vcore_reenter(void (*entry_func)(), void *stack_pointer) */

HIDDEN_ENTRY(__vcore_reenter)
  and $-16, %rsi   /* align stack */
  add $-16, %rsi   /* get a two-word buffer at the top of the stack */
  movq $0, 0(%rsi) /* clear buffer[0] */
  movq $0, 8(%rsi) /* clear buffer[1] */
  mov %rsi, %rsp   /* sys_set_stack_pointer */
  jmp %rdi         /* jump to entry_func */
PSEUDO_END(__vcore_reenter)
