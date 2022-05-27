#ifndef __BENOS_BARRIER_H_
#define __BENOS_BARRIER_H_

/*R type: .inst r opcode,func3,func7,rd,rs1,rs2  */
/* hfence.gvma rs1,rs2   #rs1为GPA,rs2为VMID */
static inline void hfence_gvma() {
    asm volatile(
        ".insn r 0x73, 0x0, 0x31, x0, x0, x0\n\t"
        ::: "memory");
}
/* hfence.vvma rs1,rs2   #rs1为GVA,rs2为ASID */
static inline void hfence_vvma() {
    asm volatile(
        ".insn r 0x73, 0x0, 0x11, x0, x0, x0\n\t"
        ::: "memory");
}
/* 修改页表之后或者设置hgatp之前需要执行HFENCE指令 */
static inline void hfence() {
	hfence_vvma();
	hfence_gvma();
}

#endif
