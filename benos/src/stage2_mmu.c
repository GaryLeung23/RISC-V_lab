#include "asm/csr.h"
#include "mm.h"
#include "asm/pgtable.h"
#include "asm/memory.h"
#include "asm/trap.h"
#include "asm/barrier.h"
#include "printk.h"
#include "asm/ptregs.h"

extern unsigned long early_pgtable_alloc(void);

/* hgatp 中的第一级页表一共需要16KB 并且要16KB对齐 */
char hs_pg_dir[PAGE_SIZE * 4] __attribute__((aligned(PAGE_SIZE *4)));

void set_stage2_page_mapping(unsigned long gpa, unsigned long hpa, unsigned long size, pgprot_t prot)
{
	__create_pgd_mapping((pgd_t *)hs_pg_dir, hpa, gpa, size, prot, early_pgtable_alloc, 0);
}

/* 设置hgatp 开启stage2 MMU*/
void write_stage2_pg_reg(void)
{
	unsigned long hgatp;
	hgatp = (((unsigned long)hs_pg_dir) >> PAGE_SHIFT) | HGATP_MODE_Sv39x4;
	hfence();
	write_csr(CSR_HGATP, hgatp);
	
	printk("write hgatp done\n");
}

static void setup_hg_page_table(unsigned long gpa, unsigned long hpa, unsigned long size, pgprot_t prot)
{
	__create_pgd_mapping((pgd_t *)hs_pg_dir, hpa, gpa, size, prot, early_pgtable_alloc, 0);
}

void stage2_page_fault(struct pt_regs *regs)
{
	unsigned long fault_addr;
	unsigned long htval, stval;
	unsigned long scause;

	/* htval保存GPA右移2位的值 */
	htval = read_csr(CSR_HTVAL);
	/* stval保存的是GVA */
	stval = read_csr(stval);
	scause = read_csr(scause);

	/* fault_addr 是GPA */
	fault_addr = (htval << 2) | (stval & 0x3);

	printk("stage2 fault addr 0x%lx, cause %lu\n", fault_addr, scause);

	fault_addr &= PAGE_MASK;

	/* 建立GPA -> HPA恒等映射 */
	/* 在stage2地址映射过程中，所有的内存访问都看作U模式的内存访问,所以需要设置页表项的U字段,所以不能使用PAGE_KERNEL*/
	setup_hg_page_table(fault_addr, fault_addr, PAGE_SIZE,
			(scause == CAUSE_STORE_GUEST_PAGE_FAULT) ? PAGE_WRITE_EXEC: PAGE_READ_EXEC);
	hfence();
}
