SECTIONS
{
	/*
        * 设置benos的加载入口地址为0x80200000
        */
	. = 0x80200000,

	/*
        * 这里是第一个段text.boot
	* 这个段存放了benos的第一条指令
        */
	_text_boot = .;
	.text.boot : { *(.text.boot) }
	_etext_boot = .;

	/*
        * text代码段
        */
	. = ALIGN(8);
	_stext = .;
	_text = .;
	.text :
       {
               *(.text)
       }
	. = ALIGN(4096);
       readonly_page = .;
       . += 4096;
	_etext = .;

	/*
        * 只读数据段
        * ADDR是内建函数,返回的是VMA
        */
	_rodata = .;
       .rodata : AT(ADDR(.rodata))
       {
               *(.rodata)
       }
       _erodata = .;

	/*
        * 数据段
        */
	_data = .;
	.data : { 
		*(.data) 
		. = ALIGN(4096);
		*(.data.init_task)
	}
	. = ALIGN(4096);
       idmap_pg_dir = .;
       . += 2 * 4096;
       idmap_pg_end = .;
	_edata = .;

	/*
        * 数据段 sdata
        */
	_sdata = .;
	. = ALIGN(4096);
	.sdata : {
		__global_pointer$ = . + 0x800;
		*(.sdata)
		*(.sbss*)
	}
	_esdata = .;

	/*
        * bss段
        *
        * ALIGN(8)表示8个字节对齐
        * bss_begin的起始地址以8字节对齐
        */
	. = ALIGN(0x8);
	_bss = .;
	.bss : { *(.bss*) } 
	_ebss = .;
	_end = .;
}

