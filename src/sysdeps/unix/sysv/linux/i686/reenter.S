#include "internal/asm.h"

/* void __vcore_reenter(void (*entry_func)(), void *stack_pointer) */

HIDDEN_ENTRY(__vcore_reenter)
  pop %eax         /* discard the return address */
  pop %eax         /* obtain entry_func */
  pop %ecx         /* obtain stack_pointer */
  add $-8, %ecx    /* get a two-word buffer at the top of the stack */
  movl $0, 0(%ecx) /* clear buffer[0] */
  movl $0, 4(%ecx) /* clear buffer[1] */
  mov %ecx, %esp   /* sys_set_stack_pointer */
  jmp %eax         /* jump to entry_func */
PSEUDO_END(__vcore_reenter)
