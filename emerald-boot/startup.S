/* startup.S -- startup, reset, and primary interrupt service routine
 *              for emerald-boot.
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * This code is designed and implemented so it can be stored in either
 * NAND or NOR, allowing the LF1000 to boot from either device.
 * As it runs, the code determines the type of boot (and therefore the address
 * of SDRam) by checking the NFBOOTENB bit in the LF1000's NAND Flash Control 
 * (NFCONTROL) register.  When the system boots from NAND, shadow mode is 
 * enabled, so SDRam is mapped at 0x00000000.  When the system boots from NOR,
 * shadow mode is disabled, and SDRam is mapped at 0x80000000.
 *
 * During a boot from NAND the LF1000 transfers the first 2K bytes from NAND
 * to SDRam at address 0x00000000.  During a boot from NOR, which is mapped
 * at address 0x00000000, the LF1000 just starts executing at address 0x00000000.
 *
 * After power-on or reset, the code (at ResetHandler) determines the type of
 * boot and branches to NorBoot or NandBoot accordingly.  Each of those sets
 * of code initializes the various stack pointers, leaving the processor in
 * supervisory mode (SVC), calls a routine that copies the (remainder of the)
 * boot code to SDRam, and then jumps to main().
 *
 * The emerald-boot code is compiled and linked with the position independent
 * code option (-fPIC).  Because of this, references to static variables are 
 * relative to the current value of the position counter register (PC).  This
 * makes it necessary to copy the boot code from NOR to SDRam very early in the
 * boot process.
 *
 * The interrupt vectors are still stored at address 0, which, after a NOR boot,
 * is in NOR.  Therefore the primary interrupt service routine, IRQHandler,
 * checks the boot type (and SDRam mapping) before calling LF_IRQHandler.
 * After a NOR boot, it calls (LF_IRQHandler + 0x80000000) so the interrupt
 * service code will be executing from SDRam and can properly access static
 * and global variables.
 */
#include <linkage.h>
#include <asm/debug-macro.S>

/*
 * Debugging stuff
 *
 * Note that these macros must not contain any code which is not
 * 100% relocatable.  Any attempt to do so will result in a crash.
 * Please select one of the following when turning on debugging.
 */

	/* inline putc, no stack needed */
	.macro	iputc,val
	mov	 r1, \val
	addruart r3
1:	waituart r2, r3
	senduart r1, r3
	busyuart r2, r3
	teq	 r1, #'\n'
	moveq	 r1, #'\r'
	beq	 1b
	.endm
		
/*
 *  StartUp is the start of the interrupt/exception vectors.
 *  It is mapped to address 0x00000000.  
 */
.section .init
.arm
.align 0

.global StartUp

.org	StartUp
	ldr	pc, reset_addr
	ldr	pc, Undef_addr	/* undefined instruction */
	ldr	pc, Undef_addr	/*SWI_addr*/	 /* SWI */
	ldr	pc, Undef_addr	/*Prefetch_addr*//* Prefetch abort */
	ldr	pc, Undef_addr	/*Dabort_addr*/	 /* Data abort */
	ldr	pc, reset_addr	/* reserved */
	ldr	pc, IRQ_addr	/* IRQ */
	ldr	pc, Undef_addr	/*FIQ_addr*/	 /* FIQ */

        
.global reset_addr
reset_addr:	.word	ResetHandler	/* Reset */
IRQ_addr:	.word	IRQHandler	/* IRQ */
Undef_addr:	.word	UndefException	/* undefined instruction */
#if 0	// Removed after replacing references (above) with Undef_addr.
	// Removed just to save a few bytes of memory.  Leaving the
	// dead code here in case we ever want/need to use it again.
SWI_addr:	.word	SWIHandler		/* SVC */
Prefetch_addr:	.word	PrefetchAbort		/* Prefetch abort */
Dabort_addr:	.word	DataAbortHandler	/* data abort */
		.word	-1
FIQ_addr:	.word	FIQHandler		/* FIQ */
#endif

.global UndefException

UndefException:
SWIHandler:
PrefetchAbort:
DataAbortHandler:
FIQHandler:

/* r4 is initialized to 0; it is set to 1 when the power button is detected
 * to be unpressed.
 * The code continues to loop until it has detected the power button
 * unpressed and then pressed.  
 * This is to make it easier for the user know that the unit powered on 
 * and then died.
 */

	mov r4, #0
#ifndef DEBUG
	/*
	 * setup UART to 115,200 bps
	 */
	ldr	r1, GPIOA_OUT	@ mimic db_init()
	ldr	r2, UART0_BASE
	ldr	r3, [r1, #32]
	bic	r3, r3, #0x30000
	orr	r3, r3, #0x10000
	str	r3, [r1, #0x20]
	mov	r3, #0x83
	strh	r3, [r2]
	mov	r3, #0x5
	strh	r3, [r2,#2]
	mov	r3, #1
	ldrh	r0, [r2, #20]
	strh	r3, [r2, #20]
	ldr	r3, UART_BAUD
	strh	r3, [r2, #68]
#endif
	b	UXEout

UXEUnpressed:
	mov     r4, #1
UXEout:
	iputc	#'U'            @ NOTE: iputc alters r1, r2, and r3
	iputc	#'X'
	iputc	#'\n'

	ldr     r0, ALIVEGPIOREADREG
	ldr     r0, [r0]
	tst     r0, #1 << 8     @ check if power button is pressed
	beq	UXEUnpressed    @ branch if not pressed
                                @ drop through if power button is pressed
	tst     r4, #1          @ check if we've seen power button released
	beq	UXEout		@ branch if we haven't seen the power btn released

    /*---------------------------------------------------------------------- 
     * The following code implements the same functionality as the die()
     * routine.  It is replicated here in case the die() routine is not
     * in ram when an unexpected exception occurs.
     *----------------------------------------------------------------------*/

	/* enable access to Alive GPIO */
    	//REG32(LF1000_ALIVE_BASE+ALIVEPWRGATEREG) = 1;
	ldr     r0, ALIVEPWRGATEREG
	mov     r3, #1
	str     r3, [r0]        @ enable access to Alive GPIO

	/* pull VDDPWRON low by resetting the flip-flop */
    	//BIT_CLR(REG32(LF1000_ALIVE_BASE+ALIVEGPIOSETREG), VDDPWRONSET);
	ldr     r1, ALIVEGPIOSETREG
	ldr     r3, [r1]
	bic     r3, r3, #128    
	str     r3, [r1]        @ clear the VDDPWRONSET bit

    	//BIT_SET(REG32(LF1000_ALIVE_BASE+ALIVEGPIORSTREG), VDDPWRONSET);
	ldr     r2, ALIVEGPIORSTREG
	ldr     r3, [r2]
	orr     r3, r3, #128    
	str     r3, [r2]        @ set the VDDPWRONRST bit

	/* reset flip-flop to latch in */
	    //REG32(LF1000_ALIVE_BASE+ALIVEGPIOSETREG) = 0;
	    //REG32(LF1000_ALIVE_BASE+ALIVEGPIORSTREG) = 0;
	mov     r3, #0
	str     r3, [r1]
	str     r3, [r2]


	/* Turn screen off */
	//gpio_configure_pin(GPIO_PORT_A, 30, GPIO_GPIOFN, 1, 0, 0);
        //void gpio_configure_pin(enum gpio_port port, enum gpio_pin pin, 
	//	    enum gpio_function f, u8 out_en, u8 pu_en, u8 val)
        //{
	//    gpio_set_fn(port, pin, f);
	ldr     r1, GPIOA_ALTFN1
	ldr     r3, [r1]
	ldr     r2, ALTFN30_MASK
	and     r3, r3, r2
	str     r3, [r1]
    
    	//    gpio_set_out_en(port, pin, out_en);
	ldr     r1, GPIOA_OUTENBL
	ldr     r3, [r1]
	ldr     r2, PIN30_MASK
	orr     r3, r3, r2
	str     r3, [r1]
    
    	//    gpio_set_pu(port, pin, pu_en);
	ldr     r1, GPIOA_PUENBL
	ldr     r3, [r1]
	ldr     r2, PIN30_MASKN
	and     r3, r3, r2
	str     r3, [r1]
    
    	//    gpio_set_val(port, pin, val);
	ldr     r1, GPIOA_OUT
	ldr     r3, [r1]
	ldr     r2, PIN30_MASKN
	and     r3, r3, r2
	str     r3, [r1]
    
        //}

    	//while(1);
	b       .

ALIVEPWRGATEREG:    .word   0xc0019000
ALIVEGPIORSTREG:    .word   0xc0019004
ALIVEGPIOSETREG:    .word   0xc0019008
ALIVEGPIOREADREG:   .word   0xc001900c

GPIOA_ALTFN1:       .word   0xc000a024
GPIOA_OUT:          .word   0xc000a000
GPIOA_OUTENBL:      .word   0xc000a004
GPIOA_PUENBL:       .word   0xc000a01c
ALTFN30_MASK:       .word   0xcfffffff
PIN30_MASK:         .word   0x40000000
PIN30_MASKN:        .word   0xbfffffff

UART0_BASE:	    .word   0xC0016000
UART_BAUD:	    .word   0x00000272

PLL_BASE:	    .word   0xC000F000
PLL_CLKMODE:	.word   0x04210480
PLL0_SET:	    .word   0x00DF2100
PLL1_SET:	    .word   0x000C4002


RamDest:   .word   0x80800000
EndCopyFromNor:   .word   _end_copy_from_nor

        // CopySize and GetCopySize() are needed only when booting from NAND.
        // They are needed because when the nand_bootstrap code references
        // _copy_end, the compiler does not always put the value in the first
        // 2K bytes of the binary image.  As a result, the code that copies
        // the remainder of the binary image from NAND to SDRam uses an
        // incorrect value for _copy_end. 
        // NOTE: this implicitly assumes that _copy_start
        // is 0x00000000, because that's where the LF1000's nand-boot 
        // code starts reading from the NAND device.
CopySize:   .word   _copy_end
.global GetCopySize
GetCopySize:
	adr	r0, CopySize	@ return the number of bytes to copy
	ldr	r0, [r0]
	bx	lr

	/* after a boot from NOR, 
	 * NOR_IRQHandler contains the address of LF_IRQHandler in SDRam.
	 * Please see this file's preamble for an explanation.
	 */	
NOR_IRQHandler:	.word	LF_IRQHandler + 0x80800000

IRQHandler:
	stmfd	sp!, {r0-r12, lr}

	ldr	r1, NFCONTROL	@ find SDRAM address
	ldr	r0, [r1]	@ get NFCONTROL value
	tst	r0, #1 << 5	@ NFBOOTENB bit
	beq	NorIRQsetup	@ branch if booted from NOR
	bl	LF_IRQHandler	@ booted from NAND, so ram is at 0x00000000
	ldmfd	sp!, {r0-r12, lr}
	subs	pc, lr, #4

NorIRQsetup:
	bl	NorIRQ		@ make a short jsr to a jump to LF_IRQHandler 'return' code will
	ldmfd	sp!, {r0-r12, lr} @ bring us back here, for the return from 
	subs	pc, lr, #4	  @ interrupt

NorIRQ:
	adr	r0, NOR_IRQHandler @ jsr to LF_IRQHandler() in SDRAM
	ldr	r0, [r0]
	mov	pc,r0		@ start executing at LF_IRQHandler (in SDRam)

// here's the original version of the code, which is less obscure.
//IRQHandler:
//	stmfd	sp!, {r0-r12, lr}
//	bl	LF_IRQHandler
//	ldmfd	sp!, {r0-r12, lr}
//	subs	pc, lr, #4


.global EnableInterrupts
EnableInterrupts:
        mrs     r0, cpsr
        bic     r0, r0, #0x80
        msr     cpsr_cxsf, r0 
	bx	lr


.global DisableInterrupts
DisableInterrupts:
        mrs     r0, cpsr
        orr     r0, r0, #0x80
        msr     cpsr_cxsf, r0
	bx	lr


    /* After power-on or reset, execution starts here */
ResetHandler:
	/* Enable I-Cache, copied from VTK/Bootloader/NandBoot/startup.s */
	MRC     p15,0,r0,c1,c0,0
	ORR     r0,r0,#(1<<12)
	MCR     p15,0,r0,c1,c0,0

	/* setup PLL0 and PLL1 while still in NOR */
	ldr	r1, PLL_BASE	@ set PLL0 and PLL1 early
	ldr	r3, PLL0_SET
	str	r3, [r1, #4]	@ PLL0
	ldr	r3, PLL1_SET
	str	r3, [r1, #8]	@ PLL1
	ldr	r3, PLL_CLKMODE
	str	r3, [r1, #0]	@ CLKMODEREG

        ldr     r0, [r1, #124]	@ PWRMODE, set CHGPLL bit
	orr	r0, r0, #1 << 15
	str	r0, [r1, #124]

PLL_WAIT:
        ldr     r0, [r1, #124]	@ PWRMODE
        tst     r0, #1 << 15    @ CHGPLL bit set?
	beq	PLL_WAIT	@ yes, wait for pll to settle

#if 1	// 10apr11	Latch power
#if 1	// after looking at disassembled C code
    	//REG32(LF1000_ALIVE_BASE+ALIVEPWRGATEREG) = 1;
    	//BIT_SET(REG32(LF1000_ALIVE_BASE+ALIVEGPIOSETREG), VDDPWRONSET);
	    //REG32(LF1000_ALIVE_BASE+ALIVEGPIOSETREG) = 0;
	    //REG32(LF1000_ALIVE_BASE+ALIVEGPIORSTREG) = 0;
	ldr     r2, ALIVEPWRGATEREG
	mov     r3, #1
	str     r3, [r2]        @ enable access to Alive GPIO
	ldr		r3, [r2, #8]	// ALIVEGPIOSETREG
	mov		r1, #0
	orr		r3, r3, #128
	str		r3, [r2, #8]
	str		r1, [r2, #8]
	str		r1, [r2, #4]
#else
    	//REG32(LF1000_ALIVE_BASE+ALIVEPWRGATEREG) = 1;
	ldr     r0, ALIVEPWRGATEREG
	mov     r3, #1
	str     r3, [r0]        @ enable access to Alive GPIO

    	//BIT_SET(REG32(LF1000_ALIVE_BASE+ALIVEGPIOSETREG), VDDPWRONSET);
	ldr     r1, ALIVEGPIOSETREG
	ldr     r3, [r1]
	orr     r3, r3, #128    
	str     r3, [r1]        @ set the VDDPWRONSET bit

	/* reset flip-flop to latch in */
	    //REG32(LF1000_ALIVE_BASE+ALIVEGPIOSETREG) = 0;
	    //REG32(LF1000_ALIVE_BASE+ALIVEGPIORSTREG) = 0;
	ldr     r2, ALIVEGPIORSTREG
	mov     r3, #0
	str     r3, [r1]
	str     r3, [r2]
#endif

#endif	// 10apr11

#ifdef DEBUG

	/*
	 * setup UART to 115,200 bps
	 */
	ldr	r1, GPIOA_OUT	@ mimic db_init()
	ldr	r2, UART0_BASE
	ldr	r3, [r1, #32]
	bic	r3, r3, #0x30000
	orr	r3, r3, #0x10000
	str	r3, [r1, #0x20]
	mov	r3, #0x83
	strh	r3, [r2]
	mov	r3, #0x5
	strh	r3, [r2,#2]
	mov	r3, #1
	ldrh	r0, [r2, #20]
	strh	r3, [r2, #20]
	ldr	r3, UART_BAUD
	strh	r3, [r2, #68]

	iputc	#'^'
#endif

	ldr	r1, NFCONTROL		@ find SDRAM address
	ldr	r0, [r1]		@ get NFCONTROL value
	tst	r0, #1 << 5		@ NFBOOTENB bit
	beq	NorBoot			@ booting from NOR
	b	NandBoot		@ booting from NAND

NFCONTROL:	.word	0xC0015874


NandBoot:
	// Initialize the stack pointers for the various processor modes.  
	// FIQ, ABT, and UND have small stacks (0x100).
	// IRQ has a 0x2D00-byte stack.  SVC (the usual mode during boot)
	// uses all but 12K (0x3000) of the stack that's reserved in the
	// emerald.lds file.  (21may09:  Currently 128K is reserved for stack.)
 	ldr	r0, =__stack_start_nand__
 	msr	cpsr_cxsf, #0xd1	/* Mode_FIQ */
 	sub	sp, r0, #0x0
 	msr	cpsr_cxsf, #0xd7	/* Mode_ABT */
 	sub	sp, r0, #0x100
 	msr	cpsr_cxsf, #0xdb	/* Mode_UND */
 	sub	sp, r0, #0x200
 	msr	cpsr_cxsf, #0xd2	/* Mode_IRQ */
 	sub	sp, r0, #0x300
 	msr	cpsr_cxsf, #0xd3	/* Mode_SVC */
 	sub	sp, r0, #0x3000
#ifdef DEBUG
	iputc	#'D'
#endif
#ifdef SELF_BOOTSTRAP
	bl	nand_bootstrap		@ finish nand boot
#endif
	b	main

RAM_ADDR_MAIN:	 	.word	main + 0x80800000		@ main in SDRAM
RAM_ADDR_db_init:	.word	db_init + 0x80800000	@ db_init in SDRAM

NorBoot:
	// Initialize the stack pointers for the various processor modes.  
	// FIQ, ABT, and UND have small stacks (0x100).
	// IRQ has a 0x2D00-byte stack.  SVC (the usual mode during boot)
	// uses all but 12K (0x3000) of the stack that's reserved in the
	// emerald.lds file.  (21may09:  Currently 128K is reserved for stack.)
 	ldr	r0, =__stack_start_nor__
 	msr	cpsr_cxsf, #0xd1	/* Mode_FIQ */
 	sub	sp, r0, #0x0
 	msr	cpsr_cxsf, #0xd7	/* Mode_ABT */
 	sub	sp, r0, #0x100
 	msr	cpsr_cxsf, #0xdb	/* Mode_UND */
 	sub	sp, r0, #0x200
 	msr	cpsr_cxsf, #0xd2	/* Mode_IRQ */
 	sub	sp, r0, #0x300
 	msr	cpsr_cxsf, #0xd3	/* Mode_SVC */
 	sub	sp, r0, #0x3000
#ifdef SELF_BOOTSTRAP
#ifdef DEBUG
	iputc	#'R'
#endif


		/* copy from NOR to SDRam w/ ldmia/stmia */ 		
	adr	r0, EndCopyFromNor
	ldr	r2, [r0]		/* put the final source address into r2 */
	adr r0, RamDest
	ldr r1, [r0]		/* put the SDRam destination address into r1 */
	sub	r0,r0			/* put the NOR source address (0) into r0 */
1:	ldmia	r0!, {r3 - r12}	/* copy from NOR to SDRam */
	stmia	r1!, {r3 - r12}

	ldmia	r0!, {r3 - r12}
	stmia	r1!, {r3 - r12}

	ldmia	r0!, {r3 - r12}
	stmia	r1!, {r3 - r12}

	ldmia	r0!, {r3 - r12}
	stmia	r1!, {r3 - r12}

	ldmia	r0!, {r3 - r12}
	stmia	r1!, {r3 - r12}

	cmp		r0, r2
	blo	  1b
	

	bl	Nor_db_init		@ make a short jsr to a jump to db_init in ram.
						@ db_init()'s 'return' code will bring us back here


@ hop to main
#ifdef DEBUG
	iputc	#'M'
#endif
	adr	r0, RAM_ADDR_MAIN		@ hop to main() in SDRAM
	ldr	r0, [r0]
	mov	pc,r0

Nor_db_init:
	adr	r0, RAM_ADDR_db_init	@ prepare to jump db_init() in SDRAM
	ldr	r0, [r0]
	mov	pc,r0					@ jump to db_init() (in SDRam)

#endif	// SELF_BOOTSTRAP


/* This is an assembly language implementation of this C routine.  
 * Using the assembly language version appears to reduce boot time by about
 * 0.1 second.

u32 cbf_checksum (u8 *buf, u32 len, u32 incoming)
{
	// printf ("crc len=%d\n", len);
	u32 *buf32 = (u32 *)buf;
	len >>= 2;
	u32 cs = incoming;

		/ * while there are at least 8 more words to process, process 8 words
		 * on each pass through this loop.
		 * NOTE: we could experiment with different "unrolling" factors,
		 *		 e.g., 4, 12, 16, 20, 24.
		 * /
	while (len >= 8)  
	{
		cs = 1 + (cs ^ *buf32++);
		cs = 1 + (cs ^ *buf32++);
		cs = 1 + (cs ^ *buf32++);
		cs = 1 + (cs ^ *buf32++);
		cs = 1 + (cs ^ *buf32++);
		cs = 1 + (cs ^ *buf32++);
		cs = 1 + (cs ^ *buf32++);
		cs = 1 + (cs ^ *buf32++);
		len -= 8;
	}
		/ * fewer than 8 more words to process.  Process one per pass. * /
	while (len--)
	{
		cs = 1 + (cs ^ *buf32++);
	}
	return cs;
}
*/
.global cbf_checksum
cbf_checksum:
			stmdb		sp!, {r4, r5, r6, r7, r8, r9, lr}    	
		 	mov			ip, r1, lsr #2
     		mov			r1, r2
    	 	b		2f

1:			ldmia		r0!, {r2-r9}
			eor			r1, r1, r2
			add			r1, r1, #1
			eor			r1, r1, r3
			add			r1, r1, #1
			eor			r1, r1, r4
			add			r1, r1, #1
			eor			r1, r1, r5
			add			r1, r1, #1
			eor			r1, r1, r6
			add			r1, r1, #1
			eor			r1, r1, r7
			add			r1, r1, #1
			eor			r1, r1, r8
			add			r1, r1, #1
			eor			r1, r1, r9
			add			r1, r1, #1			@ 17 instructions
    	 	sub			ip, ip, #8	
2:    	 	cmp			ip, #7	
    		bhi		1b
			b		4f
3:
     		ldr			r3, [r0, #-4]
     		eor			r3, r1, r3
     		add			r1, r3, #1
4:		 	sub			ip, ip, #1
     		cmn			ip, #1
     		add			r0, r0, #4
     		bne		3b
     		mov			r0, r1
			ldmia	sp!, {r4, r5, r6, r7, r8, r9, pc}

	.end
