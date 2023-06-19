#include <stdio.h>

void main()
{
	char a;
	unsigned int b;
	unsigned long c;

	a = 0x88;
	b = ~a;
	c = ~a;

	printf("a=0x%x, ~a=0x%x, b=0x%x, c=0x%lx\n", a, ~a, b, c);
}

