#include "uart.h"
#include "type.h"
#include "memset.h"
#include "printk.h"

extern void load_store_test(void);
extern void pc_related_test(void);
extern void shift_test(void);
extern void add_sub_test(void);
extern unsigned long compare_and_return(unsigned long a, unsigned long b);
extern unsigned long sel_test(unsigned long a, unsigned long b);
extern void my_memcpy_test(void);
extern void branch_test(void);

extern unsigned long func_addr[];
extern unsigned long func_num_syms;
extern char func_string;

static int print_func_name(unsigned long addr)
{
	int i;
	char *p, *string;

	for (i = 0; i < func_num_syms; i++) {
		if (addr == func_addr[i])
			goto found;
	}

	return 0;

found:
    p = &func_string;
    
    if (i == 0) {
	    string = p;
	    goto done;
    }

    while (1) {
    	p++;

    	if (*p == '\0')
    		i--;

    	if (i == 0) {
    		p++;
    		string = p;
    		break;
    	}
    }

done:
    uart_send_string(string);
    uart_send_string("\n");

    return 0;
}

void asm_test(void)
{
	unsigned long val1, val2;

	load_store_test();
	pc_related_test();
	shift_test();
	add_sub_test();

	val1 = compare_and_return(10, 9);
       if (val1 == 0)
               printk("compare_and_return ok\n");
       else 
               printk("compare_and_return fail\n");

       val2 = compare_and_return(9, 10);
       if (val2 == 0xffffffffffffffff)
               printk("compare_and_return ok\n");
       else
	       printk("compare_and_return fail\n");

       val1 = sel_test(0, 9);
       if (val1 == 11)
	       printk("sel test ok\n");

       val2 = sel_test(5, 2);
       if (val2 == 1)
	       printk("sel test ok\n");

       my_memcpy_test();
       memset((void *)0x80210005, 0x55, 40);

       branch_test();
       printk("branch test done\n");
       
       dump_stack();
}

void kernel_main(void)
{
	uart_init();
	uart_send_string("Welcome RISC-V!\r\n");
	init_printk_done();
	printk("printk init done\n");

	asm_test();

	/* lab5-4：查表*/
	print_func_name(0x800880);
	print_func_name(0x800860);
	print_func_name(0x800800);

	while (1) {
		;
	}
}
