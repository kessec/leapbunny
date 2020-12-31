OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
	. = 0x0;
	. = ALIGN(4);
	_copy_start = .;
	.text	:
	{
	  startup.o	(.init)
	  *(.text)
	}
	_etext =.;

	. = ALIGN(4);
	.data		: { *(.data) }
	_edata = .;

	. = ALIGN(4);
	.rodata : { *(.rodata) }
	. = ALIGN(4);
	.data : { *(.data) }
	. = ALIGN(0x4);
	.screens : { *(.screens) }

	. = ALIGN(0x4);
	_copy_end = .;

	. = ALIGN(4);
	 __bss_start = .;
	 __bss_start__ = .;
	.bss : { *(.bss) }
	_bss_end__ = . ;
	__bss_end__ = . ;

	/* Load kernel at 1MB, give plenty of room for emerald-boot + data */
	. = 0x00100000;
	__kernel_load__ = .;
	. = __kernel_load__ + 0x00800000;	/* 8MB max compressed image */

	. = 0x013e0000;		/* 0x20000 stack before 12MB ramdisk */
	__stack_end__ = .;
	. = __stack_end__ + 0x20000;	/* 128K stack */
	__stack_start_nand__ = .;		/* NAND stack */
	__stack_start_nor__ = . + 0x80000000; /* In NOR boot, SDRAM is 2 Gig above */

	__ramdisk_nand__ = .;
	__ramdisk_nor__  = . + 0x80000000;
}
