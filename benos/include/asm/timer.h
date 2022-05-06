#ifndef _ASM_TIMER_H
#define _ASM_TIMER_H

/*qemu virt的 SIFIVE_CLINT_TIMEBASE_FREQ = 10000000*/
#define CLINT_TIMEBASE_FREQ 10000000
#define HZ 1000

void handle_timer_irq(void);
void timer_init(void);

#endif /* _ASM_TIMER_H*/
