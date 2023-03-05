#include <stdio.h>

static inline int atomic_add(int i, unsigned long *p, unsigned long *p1)
{
	unsigned long tmp, tmp1 =1;
	int result;
	unsigned char *p2 = (unsigned char *)p;
	p2 = p2 + 4;

	printf("%llx %llx\n", (unsigned long)p, (unsigned long)p2);

	asm volatile("# atomic_add\n"
"1:	lr.w	%[tmp], (%[p])\n"
"	add	%[tmp], %[i], %[tmp]\n"
//"	sw	%[tmp], (%[p2])\n"
"	sw	%[tmp], (%[p])\n"
"	sc.w	%[result], %[tmp], (%[p])\n"
//"	bnez	%[result], 1b\n"
	: [result]"=&r" (result), [tmp]"=&r" (tmp), [p]"+r" (p), [tmp1] "=&r" (tmp1), [p2]"+r"(p2)
	: [i]"r" (i)
	: "memory");

	return result;
}

int main(void)
{
	unsigned long p = 0;
	unsigned long p1 = 0;

	int ret = atomic_add(5, &p, &p1);

	printf("atomic add: %ld, ret %d\n", p, ret);
}

