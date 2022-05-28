#include "asm/csr.h"
#include "asm/sbi.h"
#include "printk.h"
#include "type.h"
#include "asm/timer.h"
#include "io.h"
#include "asm/clint.h"
#include "asm/trap.h"

unsigned long volatile cacheline_aligned jiffies;

unsigned long vcpu_time_delta;
unsigned long vcpu_next_cycle;
int vcpu_timer_init;

/* 获取虚拟定时器的值 */
unsigned long riscv_vcpu_current_cycles()
{
	return get_cycles() + vcpu_time_delta; 
}
/* 
 *虚拟机中的虚拟定时器驱动通过调用ECALL指令，
 *陷入HS模式的VMM中，设置下一次虚拟定时器到期时间
 */
void riscv_vcpu_timer_event_start(unsigned long next_cycle)
{
	riscv_vcpu_clear_interrupt(IRQ_VS_TIMER);
	vcpu_next_cycle = next_cycle;
	//设置flag
	vcpu_timer_init = 1;
}
/* 往虚拟机中注入定时器中断 */
void riscv_vcpu_set_interrupt(int intr)
{
	csr_set(CSR_HVIP, 1UL << intr);
}
/* 向虚拟机中清除定时器中断 */
void riscv_vcpu_clear_interrupt(int intr)
{
	csr_clear(CSR_HVIP, 1UL << intr);
}

/*  
 * 判断是否已经到了虚拟定时器设置的到期时间 
 * 如果到了，则通过虚拟中断注入机制往虚拟机中注入定时器中断，即在hvip寄存器中设置VSTIP字段。
 * 虚拟机收到该中断，会在虚拟机中处理该中断
 */
void riscv_vcpu_check_timer_expired(void)
{
	unsigned long val;
	//等待VS模式中调用SBI_SET_TIMER的ecall陷入HS模式
	if (!vcpu_timer_init)
		return;
	val = riscv_vcpu_current_cycles();

	if (val < vcpu_next_cycle)
		return;

	//printk("val %lu next %lu\n", val, vcpu_next_cycle);
	/* 往虚拟机中注入定时器中断 */
	riscv_vcpu_set_interrupt(IRQ_VS_TIMER);
	//清flag
	vcpu_timer_init = 0;
}

void riscv_vcpu_timer_init(void)
{
	vcpu_time_delta = 0;
	/* 在 VS/VU 模式下读取 time 结果是真正的 host 中的 time 加上 htimedelta。 */
	write_csr(CSR_HTIMEDELTA, vcpu_time_delta);
}

#if 0
unsigned long get_cycles(void)
{
	unsigned long n;

	asm volatile (
		"rdtime %0"
		: "=r" (n));
	return n;
}
#else
unsigned long get_cycles(void)
{
	/*
	 *一般来说，mtime和mtimecmp寄存器通常只在M模式下可用，
	 *除非PMP授予U/S模式访问它们所在的内存映射区域的权限。
	 *但是对于mtime，有一个非特权的CSR寄存器去读取，那就是time 。
	 */
	return readq(VIRT_CLINT_TIMER_VAL);
	//return read_csr(time);
}
#endif

void reset_timer()
{
	//printk("get cycles %llu\n", get_cycles());
	csr_set(sie, SIE_STIE);
	/* 真正设置mtime的TIMER_CMP */
	sbi_set_timer(get_cycles() + CLINT_TIMEBASE_FREQ/HZ);
}


void timer_init(void)
{
	reset_timer();
}

/* 
 *mtime定时到之后在M模式下发生中断后中断注入到HS模式。
 *那么在HS模式下会发生定时器中断。
 *如果到了虚拟定时器设置的到期时间，则再次中断注入到虚拟机中（VS模式）
 */
void handle_timer_irq(void)
{
	csr_clear(sie, SIE_STIE);
	riscv_vcpu_check_timer_expired();
	jiffies++;
	//printk("Core0 Timer interrupt received, jiffies=%lu, %lu\n", jiffies, get_cycles());
	tick_handle_periodic();
	reset_timer();
}
