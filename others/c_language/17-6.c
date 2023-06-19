#include <stdio.h>

void main()
{
     unsigned char a = 0xa5;
     unsigned char b = ~a>>4 + 1;

     printf("b=%d\n", b);
}

