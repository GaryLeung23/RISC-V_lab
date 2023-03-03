#include <stdio.h>

static inline int atomic_add(int i, unsigned long *p, unsigned long *p1)
{
	unsigned long tmp, tmp1 =1;
	int result;

	asm volatile("# atomic_add\n"
"1:	lr.w	%[tmp], (%[p])\n"
"	add	%[tmp], %[i], %[tmp]\n"
"	sc.w	%[result], %[tmp], (%[p1])\n"
//"	bnez	%[result], 1b\n"
	: [result]"=&r" (result), [tmp]"=&r" (tmp), [p]"+r" (p), [tmp1] "=&r" (tmp1), [p1]"+r"(p1)
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

