/* Some defines for stuff found in the *.S files */

#define ENTRY(__func) \
  .globl __func; .type __func,@function; .align 1<<4; __func: cfi_startproc;
#define HIDDEN_ENTRY(__func) \
  ENTRY(__func); .hidden __func;
#define PSEUDO_END(__func) \
 cfi_endproc; .size __func,.-##__func;
#define weak_alias(__orig, __func) \
  .weak __func ; __func = __orig
#define L(__name) .L##__name
#define cfi_adjust_cfa_offset(__value) \
  .cfi_adjust_cfa_offset __value
#define cfi_def_cfa(__reg, __value) \
  .cfi_def_cfa __reg, __value
#define cfi_offset(__reg, __value) \
  .cfi_offset __reg, __value
#define cfi_startproc .cfi_startproc
#define cfi_endproc   .cfi_endproc

/*
struct user_context {
    uint64_t tf_rbx;
    uint64_t tf_rbp;
    uint64_t tf_r12;
    uint64_t tf_r13;
    uint64_t tf_r14;
    uint64_t tf_r15;
    uint64_t tf_rip;
    uint64_t tf_rsp;
    uint32_t tf_mxcsr;
    uint16_t tf_fpucw;
} __attribute__((aligned(ARCH_CL_SIZE)));
*/

#define oRBX    0
#define oRBP    8
#define oR12   16
#define oR13   24
#define oR14   32
#define oR15   40
#define oRIP   48
#define oRSP   56
#define oMXCSR 64
#define oFPUCW 68

