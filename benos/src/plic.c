#include "io.h"
#include "asm/plic.h"
#include "asm/csr.h"
#include "asm/plic.h"
#include "asm/irq.h"
#include "printk.h"

#define MAX_CPUS 1

/*根据qemu virt 平台的代码
 *include/hw/riscv/virt.h
 *VIRT_PLIC_HART_CONFIG "MS"
 *即context0是M模式,context1是S模式
 */
#define CPU_TO_HART_M(cpu) ((cpu))
#define CPU_TO_HART_S(cpu) ((cpu) + 1)

void plic_set_prority(int hwirq, int pro)
{
	unsigned int reg = PLIC_PRIORITY(hwirq);

	writel(pro, reg);
}

void plic_enable_irq(int cpu, int hwirq, int enable)
{
	unsigned int hwirq_mask = 1 << (hwirq % 32);
	int hart = CPU_TO_HART_S(cpu);
	unsigned int reg = PLIC_MENABLE(hart) + 4*(hwirq / 32);

	printk("reg: 0x%x, hwirq:%d, enable:%d\n", reg, hwirq,enable);

	if (enable)
		writel(readl(reg) | hwirq_mask, reg);
	else
		writel(readl(reg) & ~hwirq_mask, reg);
}

void plic_handle_irq(struct pt_regs *regs)
{
	int hwirq;
	/*TODO: only CPU0 handle it*/
	int hart = CPU_TO_HART_S(0);

	unsigned int claim_reg = PLIC_MCLAIM(hart);

	csr_clear(sie, SIE_SEIE);

	while((hwirq = readl(claim_reg))) {
		if (hwirq == UART0_IRQ)
			handle_uart_irq();

		writel(hwirq, claim_reg);
	}
	csr_set(sie, SIE_SEIE);
}

int plic_init(void)
{
	int i;
	int hwirq;

	for (i = 0; i < MAX_CPUS; i++) {
		/* 设置所有S模式处理器核心的中断优先级阈值为0*/
		writel(0, PLIC_MTHRESHOLD(CPU_TO_HART_S(i)));

		for (hwirq = 1; hwirq <= MAX_PLIC_IRQS; hwirq++) {
			/*关闭PLIC中所有外部中断*/
			plic_enable_irq(i, hwirq, 0);

			/* 预先设置所有中断号的优先级为1*/
			plic_set_prority(hwirq, 1);
		}
	}

	csr_set(sie, SIE_SEIE);

	return 0;
}
