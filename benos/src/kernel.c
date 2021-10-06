#include "uart.h"
#include "type.h"
#include "memset.h"
#include "printk.h"
#include "io.h"
#include "asm/sbi.h"
#include "asm/csr.h"
#include "asm/timer.h"
#include "asm/irq.h"
#include "asm/plic.h"

extern void trap_init(void);
extern void trigger_load_access_fault();

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

static void test_sysregs(void)
{
	unsigned long val;

	val = read_csr(sstatus);

	printk("sstatus =0x%x\n", val);
}

static int test_asm_goto(int a)
{
	asm goto (
			"addi %0, %0, -1\n"
			"beqz %0, %l[label]\n"
			:
			: "r" (a)
			: "memory"
			: label);

	return 0;

label:
	printk("%s: a = %d\n", __func__, a);
	return 1;
}

void inline_asm_test(void)
{
	my_memcpy_asm_test1(0x80200000, 0x80210000, 32);
	my_memcpy_asm_test2(0x80200000, 0x80210000, 32);

	/* 内嵌汇编实验3: memset函数实现*/
	memset((void *)0x80210002, 0x55, 48);

	/* 内嵌汇编实验4: 使用内嵌汇编与宏的结合*/
	my_ops_test();

	/* 内嵌汇编实验5: 实现读和写系统寄存器的宏*/
	test_sysregs();

	/* 内嵌汇编实验6: goto模板的内嵌汇编*/
	int a = 1;
	if (test_asm_goto(a) == 1)
		printk("asm_goto: return 1\n");

	int b = 0;
	if (test_asm_goto(b) == 0)
		printk("asm_goto: b is not 1\n");
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
       
       //dump_stack();
       
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

/* 通过篡改代码段里的指令代码触发一个非法指令访问异常。*/
static void create_illegal_intr(void)
{
	int *p = (int *)trigger_load_access_fault;

	*p = 0xbadbeef;
}

/* 在S模式下访问M模式下的寄存器来触发一个非法指令访问异常 */
static void access_m_reg(void)
{
	read_csr(mstatus);
}

static void trigger_access_fault(void)
{
	trigger_load_access_fault();
}

static void test_fault(void)
{
	trigger_access_fault();
}

void kernel_main(void)
{
	clean_bss();
	uart_init();
	//sbi ecall
	sbi_put_string("Welcome RISC-V!\r\n");
	init_printk_done(sbi_putchar);

	printk("printk init done\n");

	trap_init();
#ifdef CONFIG_BOARD_QEMU
	plic_init();
	enable_uart_plic();
#endif
	arch_local_irq_enable();

	asm_test();
	inline_asm_test();

	/* lab5-4：查表*/
	print_func_name(0x800880);
	print_func_name(0x800860);
	print_func_name(0x800800);

	print_mem();
	data();

	//timer_init();
	printk("sstatus:0x%lx\n", read_csr(sstatus));
	arch_local_irq_enable();
	printk("sstatus:0x%lx, sie:0x%x\n", read_csr(sstatus), read_csr(sie));
	//test_fault();

	while (1) {
		//char c = sbi_getchar();
		//if (c == '\r')
		//	printk("enter has pressed\n");
		;
	}
}
