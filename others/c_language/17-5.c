#include <stdio.h>

struct foo {
	unsigned int a:19;
	unsigned int b:13;
};

void main()
{
	struct foo addr;

	unsigned long base;

	addr.a = 0x40000;
	base = addr.a <<13;

	printf("0x%x, 0x%lx\n", addr.a <<13, base);
}

