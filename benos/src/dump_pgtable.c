#include <asm/pgtable.h>
#include "printk.h"
#include "asm/memory.h"

extern char idmap_pg_dir[];

struct addr_marker {
	unsigned long start_address;
	char *name;
};

struct prot_bits {
	unsigned long mask;
	unsigned long val;
	const char *set;
	const char *clear;
};

struct pg_level {
	const struct prot_bits *bits;
	const char *name;
	size_t num;//bits的num
	unsigned long mask;//所有bits的mask的并集
};

static const struct addr_marker address_markers[] = {
	{ 0x80200000, "Identical mapping" },
};

static const struct prot_bits pte_bits[] = {
	{
		.mask = _PAGE_SOFT,
		.val = _PAGE_SOFT,
		.set = "RSW",
		.clear = "   ",
	}, {
		.mask = _PAGE_DIRTY,
		.val = _PAGE_DIRTY,
		.set = "D",
		.clear = ".",
	}, {
		.mask = _PAGE_ACCESSED,
		.val = _PAGE_ACCESSED,
		.set = "A",
		.clear = ".",
	}, {
		.mask = _PAGE_GLOBAL,
		.val = _PAGE_GLOBAL,
		.set = "G",
		.clear = ".",
	}, {
		.mask = _PAGE_USER,
		.val = _PAGE_USER,
		.set = "U",
		.clear = ".",
	}, {
		.mask = _PAGE_EXEC,
		.val = _PAGE_EXEC,
		.set = "X",
		.clear = ".",
	}, {
		.mask = _PAGE_WRITE,
		.val = _PAGE_WRITE,
		.set = "W",
		.clear = ".",
	}, {
		.mask = _PAGE_READ,
		.val = _PAGE_READ,
		.set = "R",
		.clear = ".",
	}, {
		.mask = _PAGE_PRESENT,
		.val = _PAGE_PRESENT,
		.set = "V",
		.clear = ".",
	}
};

static struct pg_level pg_level[] = {
	{ /* pgd */
		.name	= "PGD",
		.bits	= pte_bits,
		.num	= ARRAY_SIZE(pte_bits),
	}, { /* pmd */
		.name	= "PMD",
		.bits	= pte_bits,
		.num	= ARRAY_SIZE(pte_bits),
	}, { /* pte */
		.name	= "PTE",
		.bits	= pte_bits,
		.num	= ARRAY_SIZE(pte_bits),
	},
};

/* printk 页表 prot*/
static void dump_prot(unsigned long prot, const struct prot_bits *bits,
			size_t num)
{
	unsigned i;

	for (i = 0; i < num; i++, bits++) {
		const char *s;

		if ((prot & bits->mask) == bits->val)
			s = bits->set;
		else
			s = bits->clear;

		if (s)
			printk(" %s", s);
	}
}

static int print_pgtable(unsigned long start, unsigned long end,
		int level, unsigned long val)
{
	static const char units[] = "KMGT";
	unsigned long prot = val & pg_level[level].mask;
	unsigned long delta;
	const char *unit = units;
	int i;
	
	/* printk address_markers */
	for (i = 0; i < ARRAY_SIZE(address_markers); i++)
		if (start == address_markers[i].start_address)
			printk("---[ %s ]---\n", address_markers[i].name);

	/* 过滤空页表项 */
	if (val == 0)
		return 0;

	printk("0x%016lx-0x%016lx   ", start, end);

	/* 寻找合适的单位 */
	delta = (end - start) >> 10;
	while (!(delta & 1023) && units[1]) {
		delta >>= 10;
		unit++;
	}

	printk("%9lu%c %s", delta, *unit,
			pg_level[level].name);

	if (pg_level[level].bits)
		dump_prot(prot, pg_level[level].bits,
				pg_level[level].num);
	printk("\n");

	return 0;
}

static void walk_pte(pmd_t *pmdp, unsigned long start, unsigned long end)
{
	pte_t *ptep;
	unsigned long addr = start;

	ptep = get_ptep_from_pmdp(pmdp, addr);

	do {
		print_pgtable(addr, addr + PAGE_SIZE, 2, pte_val(*ptep));
	} while (ptep++, addr += PAGE_SIZE, addr != end);
}

static void walk_pmd(pgd_t *pgdp, unsigned long start, unsigned long end)
{
	unsigned long next, addr = start;
	pmd_t *pmdp;
	pmd_t pmd;

	/* 由pgd表项和虚拟地址addr，能找到对应的PMD表项地址 */
	pmdp = get_pmdp_from_pgdp(pgdp, addr);

	do {
		pmd = *pmdp;
		/* 找到pmd表项管辖的范围的结束地址 */
		next = pmd_addr_end(addr, end);

		// 使用pmd_leaf判断huge page,如果是也打印出来
		if (pmd_none(pmd) || pmd_leaf(pmd))
			print_pgtable(addr, next, 1, pmd_val(pmd));
		else
			walk_pte(pmdp, addr, next);
	} while (pmdp++, addr = next, addr != end);
}

/* 与__create_pgd_mapping类似，只不过不需要创建页表，这里只是walk页表 */
void walk_pgd(pgd_t *pgd, unsigned long start, unsigned long size)
{
	unsigned long end = start + size;
	unsigned long next, addr = start;
	pgd_t *pgdp;
	pgd_t pgd_entry;

	/* 由虚拟地址start和PGD基地址，找到对应PGD表项地址 */
	pgdp = pgd_offset_raw(pgd, start);

	do {
		pgd_entry = *pgdp;
		/* 找到pgd表项管辖的范围的结束地址 */
		next = pgd_addr_end(addr, end);

		if (pgd_none(pgd_entry))
			print_pgtable(addr, next, 0, pgd_val(pgd_entry));
		else
			walk_pmd(pgdp, addr, next);
	} while (pgdp++, addr = next, addr != end);
}

static void pg_level_init(void)
{
	unsigned int i, j;

	for (i = 0; i < ARRAY_SIZE(pg_level); i++)
		if (pg_level[i].bits)
			for (j = 0; j < pg_level[i].num; j++)
				pg_level[i].mask |= pg_level[i].bits[j].mask;
}

extern char _text_boot[], _ebss[];
void dump_pgtable(void)
{
	unsigned long start, end;

	/*dump the memory*/
	start = PAGE_ALIGN_DOWN((unsigned long)_text_boot);
	end = PAGE_ALIGN((unsigned long)DDR_END);

	pg_level_init();
	walk_pgd((pgd_t *)idmap_pg_dir, start, end - start);
}
