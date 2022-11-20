#include "printk.h"
#include "type.h"

struct stackframe {
	unsigned long s_fp;
	unsigned long s_ra;
};

extern char _text[], _etext[];
static int kernel_text(unsigned long addr)
{
	if (addr >= (unsigned long)_text &&
	    addr < (unsigned long)_etext)
		return 1;

	return 0;
}

static void walk_stackframe(void )
{
	unsigned long sp, fp, pc;
	struct stackframe *frame;
	unsigned long low;

	const register unsigned long current_sp __asm__ ("sp");
	sp = current_sp;
	pc = (unsigned long)walk_stackframe;
	/*
	 * 获取当前栈帧的fp
	 */
	fp = (unsigned long)__builtin_frame_address(0);

	while (1) {
		if (!kernel_text(pc))
			break;

		/* 检查fp是否有效 */
		/* 
		 * 只是一个简单判断，主要是因为fp是必定要大于等于sp + 16的。
		 * 所以最简单就是先假设没有其他局部变量，这时sp+16 = fp。
		 * 如果有其他局部变量也没关系，这时sp+16+x=fp。
		 * 最终可以综合为fp >= sp + 16 = low
		 */
		low = sp + sizeof(struct stackframe);
		if ((fp < low || fp & 0xf))
			break;

		if (kernel_text(pc))
			printk("[0x%016lx - 0x%016lx]  pc 0x%016lx\n", sp, fp, pc);
		/*
		 * fp 指向上一级函数的栈底
		 * 减去16个字节，正好是struct stackframe
		 */
		frame = (struct stackframe *)(fp - 16);
		sp = fp;
		fp = frame->s_fp;

		pc = frame->s_ra - 4;


	}
}

void dump_stack(void)
{
	printk("Call Frame:\n");
	walk_stackframe();
}
