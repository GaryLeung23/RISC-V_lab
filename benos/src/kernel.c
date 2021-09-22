#include "uart.h"
#include "type.h"
#include "memset.h"
#include "printk.h"

extern void load_store_test(void);
extern void pc_related_test(void);
extern void shift_test(void);
extern void add_sub_test(void);
extern unsigned long compare_and_return(unsigned long a, unsigned long b);
extern unsigned long sel_test(unsigned long a, unsigned long b);
extern void my_memcpy_test(void);
extern void branch_test(void);
extern unsigned long macro_test_1(long a, long b);
extern unsigned long macro_test_2(long a, long b);

extern unsigned long func_addr[];
extern unsigned long func_num_syms;
extern char func_string;

static int print_func_name(unsigned long addr)
{
	int i;
	char *p, *string;

	for (i = 0; i < func_num_syms; i++) {
		if (addr == func_addr[i])
			goto found;
	}

	return 0;

found:
    p = &func_string;
    
    if (i == 0) {
	    string = p;
	    goto done;
    }

    while (1) {
    	p++;

    	if (*p == '\0')
    		i--;

    	if (i == 0) {
    		p++;
    		string = p;
    		break;
    	}
    }

done:
    uart_send_string(string);
    uart_send_string("\n");

    return 0;
}

/*
 * 内嵌汇编实验1：实现简单的memcpy函数
 *
 * 实现一个小的memcpy汇编函数
 * 从0x80200000地址拷贝32字节到0x80210000地址处，并使用gdb来比较数据是否拷贝正确
 */
static void my_memcpy_asm_test1(unsigned long src, unsigned long dst,
		unsigned long size)
{
	unsigned long tmp = 0;
	unsigned long end = src + size;

	asm volatile (
			"1: ld %1, (%2)\n"
			"sd %1, (%0)\n"
			"addi %0, %0, 8\n"
			"addi %2, %2, 8\n"
			"blt %2, %3, 1b"
			: "+r" (dst), "+r" (tmp), "+r" (src)
			: "r" (end)
			: "memory");
}

/*
 * lab7-2：实现简单的memcpy函数
 *
 * 在lab7-1的基础上尝试使用汇编符号名的方式来编写内嵌汇编
 *
 */
static void my_memcpy_asm_test2(unsigned long src, unsigned long dst,
		unsigned long size)
{
	unsigned long tmp = 0;
	unsigned long end = src + size;

	asm volatile (
			"1: ld %[tmp], (%[src])\n"
			"sd %[tmp], (%[dst])\n"
			"addi %[dst], %[dst], 8\n"
			"addi %[src], %[src], 8\n"
			"blt %[src], %[end], 1b"
			: [dst] "+r" (dst), [tmp] "+r" (tmp), [src] "+r" (src)
			: [end] "r" (end)
			: "memory");
}

#define MY_OPS(ops, asm_ops) \
static inline void my_asm_##ops(unsigned long mask, void *p) \
{                                                     \
	unsigned long tmp = 0;                            \
	asm volatile (                                \
			"ld %[tmp], (%[p])\n"              \
			" "#asm_ops" %[tmp], %[tmp], %[mask]\n"    \
			"sd %[tmp], (%[p])\n"               \
			: [p] "+r"(p), [tmp]"+r" (tmp)          \
			: [mask]"r" (mask)                   \
			: "memory"	               \
		     );                                \
}

MY_OPS(orr, or)
MY_OPS(and, and)
MY_OPS(add, add)

static void my_ops_test(void)
{
	unsigned long p;

	p = 0xf;
	my_asm_and(0x2, &p);
	printk("test and: p=0x%x\n", p);

	p = 0x80;
	my_asm_orr(0x3, &p);
	printk("test orr: p=0x%x\n", p);

	p = 0x3;
	my_asm_add(0x2, &p);
	printk("test add: p=0x%x\n", p);
}


void inline_asm_test(void)
{
	my_memcpy_asm_test1(0x80200000, 0x80210000, 32);
	my_memcpy_asm_test2(0x80200000, 0x80210000, 32);

	/* 内嵌汇编实验3: memset函数实现*/
	memset((void *)0x80210002, 0x55, 48);

	/* 内嵌汇编实验4: 使用内嵌汇编与宏的结合*/
	my_ops_test();
}

void asm_test(void)
{
	unsigned long val1, val2;

	load_store_test();
	pc_related_test();
	shift_test();
	add_sub_test();

	val1 = compare_and_return(10, 9);
       if (val1 == 0)
               printk("compare_and_return ok\n");
       else 
               printk("compare_and_return fail\n");

       val2 = compare_and_return(9, 10);
       if (val2 == 0xffffffffffffffff)
               printk("compare_and_return ok\n");
       else
	       printk("compare_and_return fail\n");

       val1 = sel_test(0, 9);
       if (val1 == 11)
	       printk("sel test ok\n");

       val2 = sel_test(5, 2);
       if (val2 == 1)
	       printk("sel test ok\n");

       my_memcpy_test();
       memset((void *)0x80210005, 0x55, 40);

       branch_test();
       printk("branch test done\n");
       
       dump_stack();
       
       val1 = macro_test_1(5, 5);
       if (val1 == 10)
	       printk("macro_test_1 ok\n");
       
       val2 = macro_test_2(5, 5);
	if (val2 == 0)
		printk("macro_test_2 ok\n");
}

extern char _text_boot[], _etext_boot[];
extern char _text[], _etext[];
extern char _rodata[], _erodata[];
extern char _data[], _edata[];
extern char _bss[], _ebss[];
extern char _sdata[], _esdata[];
extern char __global_pointer$[];

static void print_mem(void)
{
	printk("BenOS image layout:\n");
	printk("  .text.boot: 0x%08lx - 0x%08lx (%6ld B)\n",
			(unsigned long)_text_boot, (unsigned long)_etext_boot,
			(unsigned long)(_etext_boot - _text_boot));
	printk("       .text: 0x%08lx - 0x%08lx (%6ld B)\n",
			(unsigned long)_text, (unsigned long)_etext,
			(unsigned long)(_etext - _text));
	printk("     .rodata: 0x%08lx - 0x%08lx (%6ld B)\n",
			(unsigned long)_rodata, (unsigned long)_erodata,
			(unsigned long)(_erodata - _rodata));
	printk("       .data: 0x%08lx - 0x%08lx (%6ld B)\n",
			(unsigned long)_data, (unsigned long)_edata,
			(unsigned long)(_edata - _data));
	printk("        .bss: 0x%08lx - 0x%08lx (%6ld B)\n",
			(unsigned long)_bss, (unsigned long)_ebss,
			(unsigned long)(_ebss - _bss));
	printk("      .sdata: 0x%08lx - 0x%08lx (%6ld B)\n",
			(unsigned long)_sdata, (unsigned long)_esdata,
			(unsigned long)(_esdata - _sdata));
	printk("      __global_pointer$:  0x%08lx\n",(unsigned long)__global_pointer$);
}

static void clean_bss(void)
{
	unsigned long start = (unsigned long)_bss;
	unsigned long end = (unsigned long)_ebss;
	unsigned size = end - start;

	memset((void *)start, 0, size);
}

long a = 5;
long b = 10;

long data(void) {
  return a | b;
}

void kernel_main(void)
{
	clean_bss();
	uart_init();
	uart_send_string("Welcome RISC-V!\r\n");
	init_printk_done();
	printk("printk init done\n");

	asm_test();
	inline_asm_test();

	/* lab5-4：查表*/
	print_func_name(0x800880);
	print_func_name(0x800860);
	print_func_name(0x800800);

	print_mem();
	data();

	while (1) {
		;
	}
}
