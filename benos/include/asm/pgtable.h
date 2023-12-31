#ifndef _ASM_RISCV_PGTABLE_H
#define _ASM_RISCV_PGTABLE_H

#include "asm/pgtable_types.h"
#include "asm/pgtable_hwdef.h"
#include "mm.h"

/* Page protection bits */
#define _PAGE_BASE	(_PAGE_PRESENT | _PAGE_ACCESSED | _PAGE_USER)

#define PAGE_NONE		__pgprot(_PAGE_PROT_NONE)
#define PAGE_READ		__pgprot(_PAGE_BASE | _PAGE_READ)
#define PAGE_WRITE		__pgprot(_PAGE_BASE | _PAGE_READ | _PAGE_WRITE)
#define PAGE_EXEC		__pgprot(_PAGE_BASE | _PAGE_EXEC)
#define PAGE_READ_EXEC		__pgprot(_PAGE_BASE | _PAGE_READ | _PAGE_EXEC)
#define PAGE_WRITE_EXEC		__pgprot(_PAGE_BASE | _PAGE_READ |	\
					 _PAGE_EXEC | _PAGE_WRITE)

#define PAGE_COPY		PAGE_READ
#define PAGE_COPY_EXEC		PAGE_EXEC
#define PAGE_COPY_READ_EXEC	PAGE_READ_EXEC
#define PAGE_SHARED		PAGE_WRITE
#define PAGE_SHARED_EXEC	PAGE_WRITE_EXEC

#define _PAGE_KERNEL		(_PAGE_READ \
				| _PAGE_WRITE \
				| _PAGE_PRESENT \
				| _PAGE_ACCESSED \
				| _PAGE_DIRTY \
				| _PAGE_GLOBAL)

#define PAGE_KERNEL		__pgprot(_PAGE_KERNEL)
#define PAGE_KERNEL_READ	__pgprot(_PAGE_KERNEL & ~_PAGE_WRITE)
#define PAGE_KERNEL_EXEC	__pgprot(_PAGE_KERNEL | _PAGE_EXEC)
#define PAGE_KERNEL_READ_EXEC	__pgprot((_PAGE_KERNEL & ~_PAGE_WRITE) \
					 | _PAGE_EXEC)

#define PAGE_TABLE		__pgprot(_PAGE_TABLE)

/* 查找PGD索引 */
#define pgd_index(addr) (((addr) >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1))

/* 由虚拟地址addr和pgd基地址，找到对应PGD表项地址 */
#define pgd_offset_raw(pgd, addr) ((pgd) + pgd_index(addr))

/* 通过地址addr与end  找到pgd表项管辖的范围的结束地址 */
#define pgd_addr_end(addr, end)						\
({	unsigned long __boundary = ((addr) + PGDIR_SIZE) & PGDIR_MASK;	\
	(__boundary - 1 < (end) - 1) ? __boundary : (end);		\
})

#define pmd_addr_end(addr, end)						\
({	unsigned long __boundary = ((addr) + PMD_SIZE) & PMD_MASK;	\
	(__boundary - 1 < (end) - 1) ? __boundary : (end);		\
})

#define pgd_none(pgd) (!pgd_val(pgd))
#define pmd_none(pmd) (!pmd_val(pmd))
#define pte_none(pte) (!pte_val(pte))

/* 根据pgd的内容获得下一级页表基地址*/
static inline unsigned long pgd_page_paddr(pgd_t pgd)
{
	return (pgd_val(pgd) >> _PAGE_PFN_SHIFT) << PTE_SHIFT;
}

/* 查找PMD索引 */
#define pmd_index(addr) ((addr) >> PMD_SHIFT & (PTRS_PER_PMD - 1))
/* 由pgdp表项地址和虚拟地址addr，能找到对应的pmdp表项地址 */
#define pmd_offset_phys(pgdp, addr) ((pmd_t *)(pgd_page_paddr(*(pgdp)) + pmd_index(addr) * sizeof(pmd_t)))
/* pmd_offset_phys宏的函数形式。*/
static inline pmd_t *get_pmdp_from_pgdp(pgd_t *pgdp, unsigned long va)
{
	pmd_t *pmd_base;

	/* 由虚拟地址va，能找到对应的PMD表项的偏移offset */
	unsigned long index = pmd_index(va);

	/* 由PGD表项的内容，解码出下一级页表PMD的基地址 */
	pmd_base = (pmd_t *)pgd_page_paddr(*pgdp);

	/* PMD的基地址加上index，得到对应的pmd表项的地址*/
	return pmd_base + index;
}

/* 根据pmd的内容获得下一级页表基地址*/
static inline unsigned long pmd_page_paddr(pmd_t pmd)
{
	return (pmd_val(pmd) >> _PAGE_PFN_SHIFT) << PTE_SHIFT;
}

/* 查找PTE索引 */
#define pte_index(addr) (((addr) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))

/* 通过地址va与pmdp地址,获得对应ptep的地址。*/
static inline pte_t *get_ptep_from_pmdp(pmd_t *pmdp, unsigned long va)
{
	pte_t *pte_base;
	/* 由虚拟地址va，能找到对应的PTE表项的偏移offset */

	unsigned long index = pte_index(va);

	/* 由PMD表项的内容，解码出下一级页表PTE的基地址 */
	pte_base = (pte_t *)pmd_page_paddr(*pmdp);

	/* PTE的基地址加上index，得到对应的pte表项的地址*/
	return pte_base + index;
}

static inline void set_pgd(pgd_t *pgdp, pgd_t pgd)
{
	*pgdp = pgd;
}

static inline void set_pmd(pmd_t *pmdp, pmd_t pmd)
{
	*pmdp = pmd;
}

static inline void set_pte(pte_t *ptep, pte_t pte)
{
	*ptep = pte;
}

/* pfn --> pgd */
static inline pgd_t pfn_pgd(unsigned long pfn, pgprot_t prot)
{
	return __pgd((pfn << _PAGE_PFN_SHIFT) | pgprot_val(prot));
}

static inline unsigned long _pgd_pfn(pgd_t pgd)
{
	return pgd_val(pgd) >> _PAGE_PFN_SHIFT;
}

/* pfn --> pmd */
static inline pmd_t pfn_pmd(unsigned long pfn, pgprot_t prot)
{
	return __pmd((pfn << _PAGE_PFN_SHIFT) | pgprot_val(prot));
}

static inline unsigned long _pmd_pfn(pmd_t pmd)
{
	return pmd_val(pmd) >> _PAGE_PFN_SHIFT;
}

/* pfn --> pte */
static inline pte_t pfn_pte(unsigned long pfn, pgprot_t prot)
{
	return __pte((pfn << _PAGE_PFN_SHIFT) | pgprot_val(prot));
}

static inline int pmd_present(pmd_t pmd)
{
       return (pmd_val(pmd) & _PAGE_PRESENT );
}

static inline int pmd_leaf(pmd_t pmd)
{
       return pmd_present(pmd) && (pmd_val(pmd) & _PAGE_LEAF);
}

static inline int pte_write(pte_t pte)
{
	return pte_val(pte) & _PAGE_WRITE;
}

static inline int pte_exec(pte_t pte)
{
	return pte_val(pte) & _PAGE_EXEC;
}

static inline int pte_dirty(pte_t pte)
{
	return pte_val(pte) & _PAGE_DIRTY;
}

static inline int pte_young(pte_t pte)
{
	return pte_val(pte) & _PAGE_ACCESSED;
}

static inline pte_t pte_wrprotect(pte_t pte)
{
	return __pte(pte_val(pte) & ~(_PAGE_WRITE));
}

static inline pte_t pte_mkwrite(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_WRITE);
}

static inline pte_t pte_mkdirty(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_DIRTY);
}

static inline pte_t pte_mkclean(pte_t pte)
{
	return __pte(pte_val(pte) & ~(_PAGE_DIRTY));
}

static inline pte_t pte_mkyoung(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_ACCESSED);
}

void dump_pgtable(void);
void walk_pgd(pgd_t *pgd, unsigned long start, unsigned long size);

#endif
