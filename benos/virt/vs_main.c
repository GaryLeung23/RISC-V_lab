#include "asm/csr.h"
#include "asm/sbi.h"
#include "vs_mm.h"
#include "mm.h"
#include "asm/pgtable.h"
#include "asm/memory.h"
#include "asm/barrier.h"
#include "printk.h"
#include "mm.h"
#include "vs_mmu.h"
#include "asm/irq.h"

char vs_stack[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));

extern void vs_trap_init(void);

void vs_main()
{
	unsigned long val;
	unsigned long gpa_addr, gva_addr;
	
	/* 设置sstatus.SPP */
	val = read_csr(sstatus);
	val |= SSTATUS_SPP;
	write_csr(sstatus, val);

	/* 设置hstatus.SPV hstatus.SPVP 这样sret之后就是vs模式*/
	val = read_csr(CSR_HSTATUS);
	val |= HSTATUS_SPV | HSTATUS_SPVP;
	write_csr(CSR_HSTATUS, val);
	printk("hstatus 0x%lx\n", val);
	
	gpa_addr = get_free_page();
	gva_addr = VS_MEM;

	/* 这里是直接设置物理地址的值 */
	*(unsigned long *)gpa_addr = 0x12345678;

	printk("gva 0x%lx, gpa 0x%lx  *gpa 0x%lx\n",
			gva_addr, gpa_addr, *(unsigned long *)gpa_addr);

	
	//set_stage2_page_mapping(gpa_addr, gpa_addr, PAGE_SIZE, PAGE_WRITE_EXEC);

	/*开启stage2 MMU 但是页表项都是空的*/
	write_stage2_pg_reg();

	printk("...entering VM...\n");
	
	riscv_vcpu_timer_init();
	/* 进入vs模式 */
	/* 从进入vs模式开始，虽然stage1没有开启,但是stage2已经开启，所以每条指令都可能触发guest-page fault(只要地址对应的stage2页表为空) */
	/* 第一条发出guest-page fault的指令就是jump_to_vs_mode中的ret指令*/
	/* 在HS模式中处理guest-page fault,分配stage 2对应地址的页表*/
	/* 从HS到VS,需要切换SP */
	gpa_addr = jump_to_vs_mode((unsigned long)vs_stack + PAGE_SIZE, gpa_addr);

	vs_trap_init();
	/* 建立关于 代码段 与 DDR 的stage1的地址映射关系 这里还是恒等映射*/
	/* 这里会开启stage 1的地址映射,从这里开始，虚拟机上的地址就要经过两阶段映射。GVA->GPA(HVA)->HPA*/
	vs_paging_init();
	
	// gva_addr = VS_MEM; 这里是多余的？
	/* 这里建立gva_addr ->gpa_addr的非恒等映射 */
	set_vs_mapping_page(gva_addr, gpa_addr);
	printk("running in VS mode\n");
	printk("sstatus 0x%lx\n", read_csr(sstatus));
	/* 这里从gva_addr读取的内容如果是设置到 一开始物理地址上的内容0x12345678,就说明两阶段地址映射成功*/
	printk("*gva_ddr 0x%lx\n", *(unsigned long *)gva_addr);

	printk("...exit VM...\n");
	SBI_CALL_0(SBI_EXIT_VM_TEST);
	printk("...back to VM...\n");

	printk("*gva_ddr 0x%lx\n", *(unsigned long *)gva_addr);

	while (1)
		;
}
