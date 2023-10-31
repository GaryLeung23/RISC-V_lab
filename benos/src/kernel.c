#include "uart.h"

extern void load_store_test(void);
extern void pc_related_test(void);
extern void shift_test(void);
extern void add_sub_test(void);
extern void compare_test(void);

void asm_test(void)
{
    load_store_test();
    pc_related_test();
    shift_test();
    add_sub_test();
    compare_test();
}

void kernel_main(void)
{
    uart_init();
    uart_send_string("Welcome RISC-V!\r\n");

    asm_test();

    while (1) {
        ;
    }
}
