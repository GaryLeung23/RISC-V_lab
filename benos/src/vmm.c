/*R type: .inst r opcode,func3,func7,rd,rs1,rs2  */
/* hlv.ws */
unsigned long hlvwu(unsigned long addr)
{
    unsigned long value;
    asm volatile(
        ".insn r 0x73, 0x4, 0x34, %0, %1, x1\n\t"
        : "=r"(value)
        : "r"(addr) 
        : "memory", "x1");
    return value;
}

/* hsv.w */
void hsvw(unsigned long addr, unsigned long value)
{
    asm volatile(
        ".insn r 0x73, 0x4, 0x35, x0, %1, %0\n\t"
        : "+r"(value)
        : "r"(addr) 
        : "memory");
}