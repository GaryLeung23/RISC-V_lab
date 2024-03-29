#include "asm/pgtable.h"
#include "asm/memory.h"
#include "vs_mm.h"
#include "memset.h"
#include "asm/memory.h"
#include "printk.h"
#include "asm/csr.h"
#include "asm/uart.h"
#include "asm/clint.h"

/* vsatp 中的第一级页表一共需要4KB 并且要4KB对齐 */
char vs_page_table_dir[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));;

/*
 * alloc_init_pte: 检查pmdp表项内容，如果为空，则分配PTE页表，并回填pmdp表项
 *					并且为需要用到的PTE页表项根据prot与PFN填写
 * 
 * pmdp: 指定PMD页表地址
 * addr: 虚拟起始地址
 * end: 虚拟结束地址
 * phys: 物理起始地址
 * prot: 页表项PTE的属性
 * alloc_pgtable: 分配各级页表的分配函数
 * flags: 标志位
 */
static void alloc_init_pte(pmd_t *pmdp, unsigned long addr,
		unsigned long end, unsigned long phys,
		pgprot_t prot,
		unsigned long (*alloc_pgtable)(void),
		unsigned long flags)
{
	pmd_t pmd = *pmdp;
	pte_t *ptep;

	/*pmd表项内容为空，说明对应的PTE页表还没建立*/
	if (pmd_none(pmd)) {
		unsigned long pte_phys;

		/*分配一个page，用于PTE页表*/
		pte_phys = alloc_pgtable();
		/* 由PTE基地址来回填pmdp表项 */
		set_pmd(pmdp, pfn_pmd(PFN_DOWN(pte_phys), PAGE_TABLE));
		pmd = *pmdp;
	}

	/* 由pmdp表项地址和虚拟地址addr，能找到对应的ptep表项地址 */
	ptep = get_ptep_from_pmdp(pmdp, addr);
	do {
		/* 设置PTE*/
		set_pte(ptep, pfn_pte(PFN_DOWN(phys), prot));
		phys += PAGE_SIZE;
	} while (ptep++, addr += PAGE_SIZE, addr != end);
}
/*
 * alloc_init_pmd: 检查pgdp表项内容，如果为空，则分配PMD页表，并回填pgdp表项
 *                 并且为需要用到的PMD页表项也调用alloc_init_pte
 *
 * pgdp: 指定PGD页表地址
 * addr: 虚拟起始地址
 * end: 虚拟结束地址
 * phys: 物理起始地址
 * prot: 页表项PTE的属性
 * alloc_pgtable: 分配各级页表的分配函数
 * flags: 标志位
 */
static void alloc_init_pmd(pgd_t *pgdp, unsigned long addr,
		unsigned long end, unsigned long phys,
		pgprot_t prot,
		unsigned long (*alloc_pgtable)(void),
		unsigned long flags)
{
	pgd_t pgd = *pgdp;
	pmd_t *pmdp;
	unsigned long next;

	/*pgd表项内容为空，说明对应的PMD页表还没建立*/
	if (pgd_none(pgd)) {
		unsigned long pmd_phys;

		/*分配一个page，用于PMD页表*/
		pmd_phys = alloc_pgtable();
		/* 由PMD基地址来回填pgdp表项 */
		set_pgd(pgdp, pfn_pgd(PFN_DOWN(pmd_phys), PAGE_TABLE));
		pgd = *pgdp;
	}

	/* 由pgd表项和虚拟地址va，能找到对应的PMD表项地址 */
	pmdp = get_pmdp_from_pgdp(pgdp, addr);
	do {
		next = pmd_addr_end(addr, end);

		alloc_init_pte(pmdp, addr, next, phys,
					prot,  alloc_pgtable, flags);

		phys += next - addr;
	} while (pmdp++, addr = next, addr != end);
}

/*
 * __create_vs_pgd_mapping: 创建页表
 *
 * pgdir: PGD页表的基地址
 * phys: 物理地址PA
 * virt: 虚拟地址VA
 * size: 映射的大小
 * prot: 页表项PTE的属性
 * alloc_pgtable: 分配各级页表的分配函数
 * flags: 标志位
 */
static void __create_vs_pgd_mapping(pgd_t *pgdir, unsigned long phys,
		unsigned long virt, unsigned long size,
		pgprot_t prot,
		unsigned long (*alloc_pgtable)(void),
		unsigned long flags)
{
	/* 由虚拟地址va和PGD基地址，找到对应PGD表项地址 */
	pgd_t *pgdp = pgd_offset_raw(pgdir, virt);
	unsigned long addr, end, next;

	phys = PAGE_ALIGN_DOWN(phys);
	addr = PAGE_ALIGN_DOWN(virt);
	end = PAGE_ALIGN_UP(virt + size);

	do {
		/* 找到pgd表项管辖的范围的结束地址 */
		next = pgd_addr_end(addr, end);
		/* 先分配下一级页表项,再将其地址填写到上一级的页表项中 */
		alloc_init_pmd(pgdp, addr, next, phys,
				prot, alloc_pgtable, flags);
		phys += next - addr;
	} while (pgdp++, addr = next, addr != end);
}

/*
 * 分配一个page用于各级页表
 */
static unsigned long early_pgtable_alloc(void)
{
	unsigned long phys;

	phys = get_free_page();
	memset((void *)phys, 0, PAGE_SIZE);

	return phys;
}

/* 创建gva ->gpa 的stage 1的页表 如果gva不等于gpa就不是恒等映射*/
void set_vs_mapping_page(unsigned long gva, unsigned long gpa)
{
	/* map vs page */
	__create_vs_pgd_mapping((pgd_t *)vs_page_table_dir, gpa, gva,
			PAGE_SIZE, PAGE_KERNEL,
			early_pgtable_alloc,
			0);
	asm volatile ("sfence.vma \n\t");
}

extern char _text_boot[], _etext[], _end[];

/* 建立stage1的地址映射关系 这里还是恒等映射*/
void vs_paging_init(void)
{
	unsigned long val;

	unsigned long start;
	unsigned long end;

	/*map text*/
	start = (unsigned long)_text_boot;
	end = (unsigned long)_etext;
	__create_vs_pgd_mapping((pgd_t *)vs_page_table_dir, start, start,
			end - start, PAGE_KERNEL_READ_EXEC,early_pgtable_alloc,0);

	/*map DDR*/
	start = PAGE_ALIGN_UP((unsigned long)_etext);
	end = DDR_END;
	__create_vs_pgd_mapping((pgd_t *)vs_page_table_dir, start, start,
			end - start, PAGE_KERNEL, early_pgtable_alloc, 0);

	/*map UART*/
	start = UART;
	__create_vs_pgd_mapping((pgd_t *)vs_page_table_dir, start, start,
			UART_SIZE, PAGE_KERNEL, early_pgtable_alloc, 0);

	
	/* map CLINT */
	start = VIRT_CLINT_ADDR;
	__create_vs_pgd_mapping((pgd_t *)vs_page_table_dir, start, start,
			VIRT_CLINT_SIZE, PAGE_KERNEL, early_pgtable_alloc, 0);

	/* 设置vsatp 开启stage 1 MMU */
	val = (((unsigned long)vs_page_table_dir) >> PAGE_SHIFT) | SATP_MODE_39;
	asm volatile ("sfence.vma \n\t");
	write_csr(satp, val);
}
