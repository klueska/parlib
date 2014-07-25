#ifndef _ASM_FSGS_H
#define _ASM_FSGS_H 1

static inline unsigned long rdgsbase(void)
{
	unsigned long gs;
	asm volatile(".byte 0xf3,0x48,0x0f,0xae,0xc8 # rdgsbaseq %%rax"
			: "=a" (gs)
			:: "memory");
	return gs;
}

static inline unsigned long rdfsbase(void)
{
	unsigned long fs;
	asm volatile(".byte 0xf3,0x48,0x0f,0xae,0xc0 # rdfsbaseq %%rax"
			: "=a" (fs)
			:: "memory");
	return fs;
}

static inline void wrgsbase(unsigned long gs)
{
	asm volatile(".byte 0xf3,0x48,0x0f,0xae,0xd8 # wrgsbaseq %%rax"
			:: "a" (gs)
			: "memory");
}

static inline void wrfsbase(unsigned long fs)
{
	asm volatile(".byte 0xf3,0x48,0x0f,0xae,0xd0 # wrfsbaseq %%rax"
			:: "a" (fs)
			: "memory");
}

#endif
