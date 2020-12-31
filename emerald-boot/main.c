/* main.c -- Emerald Platform Bootloader
 *
 * Copyright 2007-2010 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 * Scott Esters <sesters@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "include/autoconf.h"	/* for partition info */
#include "include/mach-types.h" /* for machine info */
#include <mach/platform.h>
#include <mach/common.h>
#include <mach/uart.h>
#include <mach/nand.h>
#include <mach/gpio.h>
#include <mach/gpio_hal.h>
#include <mach/gpio_map.h>	/* GPIO button mappings */
#include <mach/clkpwr.h>
#include <stdint.h>
#include <mach/adc.h>

#include "include/board.h"
#include "include/string.h"
#include "include/setup.h"
#include "include/debug.h"
#include "include/nand.h"
#include "include/cbf.h"
#include "include/display.h"
#include "include/clock.h"
#include "include/gpio.h"
#include "include/adc.h"
#include "include/ram.h"
#include "include/rle.h"
#include "include/screens.h"
#include "include/versions.h"
#include "include/led_config.h"
#include "include/buttons.h"
#include "include/bbtest.h"

#if HNS_SIZE == 0
#error "HNS_SIZE not known"
#endif

#undef	LOW_BATTERY_NO_BOOT
#define LOW_BATTERY_SHOW_SCREEN

void wait_for_usb_host(int board_id); // Never returns

/* static buffers: 
 * These are automatically put into hi or lo memory (thus, into real RAM)
 * by compiler runtime code.  I don't know how, but it is nice since I
 * don't need to relocate them myself with calc_SDRAM_ADDRESS() 
*/
static u32 nandio_buffer[WORST_EB_SIZE/sizeof(u32)];
#define CMDLINE_LEN	4096
static char cmdline_buffer[CMDLINE_LEN];
char *get_cmdline_buffer () { return &cmdline_buffer[0]; }

#ifdef DEBUG_NAND_ID
extern char mybuf[1024];
#endif

/* USB controller */
#define UDC_PCR		0x52	/* PCR register offset */
#define PCE		0	/* not-enable bit */

u32 calc_SDRAM_ADDRESS();
u32 calc_PARAMS_ADDRESS();

/*
 * Die in case of unrecoverable error.  On LF1000, we pull the power off. 
 * Otherwise just lock up. 
 */
void die(void)
{
	db_puts("die()\n");

	/* enable access to Alive GPIO */
	REG32(LF1000_ALIVE_BASE+ALIVEPWRGATEREG) = 1;
	/* pull VDDPWRON low by resetting the flip-flop */
	BIT_CLR(REG32(LF1000_ALIVE_BASE+ALIVEGPIOSETREG), VDDPWRONSET);
	BIT_SET(REG32(LF1000_ALIVE_BASE+ALIVEGPIORSTREG), VDDPWRONSET);
	/* reset flip-flop to latch in */
	REG32(LF1000_ALIVE_BASE+ALIVEGPIOSETREG) = 0;
	REG32(LF1000_ALIVE_BASE+ALIVEGPIORSTREG) = 0;
	/* power should be off now... */

	/* Turn screen off */
	gpio_configure_pin(GPIO_PORT_A, 30, GPIO_GPIOFN, 1, 0, 0);

	while(1);
}

/*
 * Clean up before booting Linux on the ARM926: turn off instruction cache and 
 * make sure data cache doesn't contain any stale data. 
 */
/* static */ /* made global so it can be called from wait_for_usb_host() */
void cleanup_for_linux(void)
{
#define C1_DC	(1<<2)	/* dcache off/on */
#define C1_IC	(1<<12)	/* icache off/on */
	unsigned long i;

	/* turn off I/D-cache */
	asm ("mrc p15, 0, %0, c1, c0, 0":"=r" (i));
	i &= ~(C1_DC | C1_IC);
	asm ("mcr p15, 0, %0, c1, c0, 0": :"r" (i));

	/* flush I/D-cache */
	i = 0;
	asm ("mcr p15, 0, %0, c7, c7, 0": :"r" (i));
}

/* 
 * build_params -- set up parameters for the kernel uImage 
 */
void build_params(char *cmdline, struct tag *params)
{
	char *p;

	/* 
	 * set up the core tag 
	 */
	db_puts ("build_params at ");
	db_int ((unsigned int)params);
	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size(tag_core);
	params->u.core.flags = 0;
	params->u.core.pagesize = 0;
	params->u.core.rootdev = 0;
	params = tag_next(params);

	/* 
	 * set up the kernel command line tag 
	 */

	if(cmdline == 0)
		goto END_TAG;
	
	/* eat leading spaces */
	for(p = cmdline; *p == ' '; p++);

	if(*p == '\0')
		goto END_TAG;

	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size = (sizeof(struct tag_header) + strlen(p) + 1 + 4) >> 2;
	strcpy(params->u.cmdline.cmdline, p);
	params = tag_next(params);

	/*
	 * set up the end tag
	 */
END_TAG:
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;

	db_puts ("-");
	db_int ((unsigned int)tag_next(params));
	db_puts ("\n");
}

#ifdef RAMDISK
extern u8 __ramdisk_nand__[];
extern u8 __ramdisk_nor__[];
u32 GetRamDiskAddr() {
	if (IS_CLR(REG32(LF1000_MCU_S_BASE + NFCONTROL), NFBOOTENB))
		/* NOR BOOT, shadow disabled */
		return (u32)__ramdisk_nor__;
	else
		/* NAND BOOT, shadow enabled */
		return (u32)__ramdisk_nand__;
}
#endif


u32 calc_SDRAM_ADDRESS()
{
	if (IS_CLR(REG32(LF1000_MCU_S_BASE + NFCONTROL), NFBOOTENB))
		/* NOR BOOT, shadow disabled */
		return(0x80000000);	/* TODO: remove hardcoded values */
	else
		/* NAND BOOT, shadow enabled */
		return(0x00000000);	/* TODO: remove hardcoded values */	
}

u32 calc_PARAMS_ADDRESS()
{
	// See params.c
	extern char my_kernel_params[0x800];
	return (u32)my_kernel_params + calc_SDRAM_ADDRESS();
}

/* 
 * Fetch and return the address of the kernel entry point.  
 * If the kernel has not already been read into SDRam, or if for any other
 * reason its entry point is not available, this routine returns 0.
 */
static u32 load_kernel(char *cmdline, u32 *params_buffer)
{
	void *entry_point;
	build_params(cmdline, (struct tag *)params_buffer);

	/* Get the entry point */
	if (0 != cbf_get_jump_address (&entry_point)) {
    	db_puts ("Failed to get kernel entry_point\n");
        return 0;
    }
	db_puts ("entry_point=");
	db_int ((unsigned int)entry_point);
	db_puts ("\n");
	return (u32)entry_point;
}

/*
 * Update power state to reflect boot status
 */
static enum scratch_power load_power_state(void)
{
	enum scratch_power id, next;
	int scratch;

	scratch = gpio_get_scratchpad();

	/* get last boot status */
	id = (scratch >> SCRATCH_POWER_POS) & BIT_MASK_ONES(SCRATCH_POWER_SIZE);
	if(id == SCRATCH_POWER_FIRSTBOOT)
		next = SCRATCH_POWER_COLDBOOT;
	else
		next = SCRATCH_POWER_WARMBOOT;

	/* clear out last value, then place new value in */
	scratch &= ~(BIT_MASK_ONES(SCRATCH_POWER_SIZE) 
		     << SCRATCH_POWER_POS);
	scratch |= ((next & BIT_MASK_ONES(SCRATCH_POWER_SIZE))
		    << SCRATCH_POWER_POS);
	gpio_set_scratchpad(scratch);
	
	return id;
}

/*
 * Set the boot_image to reflect which kernel we actually booted
 */
static void set_boot_image(enum scratch_boot_image id)
{
	int scratch;

	scratch = gpio_get_scratchpad();
	/* clear out last value, then place new value in */
	scratch &= ~(BIT_MASK_ONES(SCRATCH_BOOT_IMAGE_SIZE) 
		     << SCRATCH_BOOT_IMAGE_POS);
	scratch |= ((id & BIT_MASK_ONES(SCRATCH_BOOT_IMAGE_SIZE))
		    << SCRATCH_BOOT_IMAGE_POS);
	gpio_set_scratchpad(scratch);
}

/* Get boot request */
static enum scratch_request get_request(void)
{
	return ((gpio_get_scratchpad() >> SCRATCH_REQUEST_POS) &
		BIT_MASK_ONES(SCRATCH_REQUEST_SIZE));
}

/* Set boot request */
static void set_request(enum scratch_request value)
{
	u32 scratch = gpio_get_scratchpad();	// get bits
						// remove bits
	scratch &= ~(BIT_MASK_ONES(SCRATCH_REQUEST_SIZE) 
		     << SCRATCH_REQUEST_POS);
	scratch |= (value << SCRATCH_REQUEST_POS);
	gpio_set_scratchpad(scratch);
}

/* Get panic */
static int get_panic(void)
{
	return ((gpio_get_scratchpad() >> SCRATCH_PANIC_POS) &
		BIT_MASK_ONES(SCRATCH_PANIC_SIZE));
}

/*
 * Retrive old shutdown reason, set shutdown bit to 'DIRTY'
 */
static enum scratch_shutdown load_shutdown_state()
{
	enum scratch_shutdown id;
	u32 scratch;

	scratch = gpio_get_scratchpad();

	/* get last shutdown status */
	id = (scratch >> SCRATCH_SHUTDOWN_POS) 
          & BIT_MASK_ONES(SCRATCH_SHUTDOWN_SIZE);

	/* reset shutdown bit to 'DIRTY', Linux sets to 'CLEAN' at shutdown */
	scratch &= ~(BIT_MASK_ONES(SCRATCH_SHUTDOWN_SIZE) << SCRATCH_SHUTDOWN_POS);
	scratch |= (SCRATCH_SHUTDOWN_DIRTY << SCRATCH_SHUTDOWN_POS);
	gpio_set_scratchpad(scratch);

	return id;	// return prior shutdown status
}

/*
 * Set boot source -- show if LF1000 booted from 'NAND', 'NOR', or 'UART'
 * Based on CfgBOOTMODE1 (SD[6] J13-2) and CfgBOOTMODE0 (SD[5] J13-3)
 *  Boot Mode   SD[6] SD[5]
 *     UART        0     0   Load 16K bytes from UART 0 RX (J13-20) into SDRAM
 *     NAND        0     1   Standard boot
 *     ROM         1     0   NOR ROM boot (SDRAM at 0x80000000)
 *     NONE        1     1
 *
 * Run before UART divisor gets set, as it is set to 19,200bps for UART boot
 */
static int load_boot_source(u16 bitrate, int usb)
{
	enum scratch_boot_source id;
	u32 boot;
	int scratch;

    boot = IS_SET(REG32(LF1000_MCU_S_BASE+NFCONTROL),NFBOOTENB);
	if (usb)
		id = SCRATCH_BOOT_SOURCE_USB;
    else if (!boot)
		id = SCRATCH_BOOT_SOURCE_NOR;
    else if (bitrate) 	/* check UART Bit Rate Divisor */
		id = SCRATCH_BOOT_SOURCE_UART;	// UART boot sets divisor 
	else
		id = SCRATCH_BOOT_SOURCE_NAND;

	/* set boot source */
	scratch = gpio_get_scratchpad();
	scratch &= ~(BIT_MASK_ONES(SCRATCH_BOOT_SOURCE_SIZE)
		 	     << SCRATCH_BOOT_SOURCE_POS);
	scratch |= (id << SCRATCH_BOOT_SOURCE_POS);
	gpio_set_scratchpad(scratch);

	return id;	// return boot source
}

static int load_board_id(void)
{
	u32 id, scratch;
	int i;

	/* make pins inputs */
	for(i = GPIO_CFG_HIGH; i >= GPIO_CFG_LOW; i--)
		gpio_configure_pin(GPIO_CFG_PORT, i, GPIO_GPIOFN, 0, 0, 0);

	/* read pins */
	for(id = 0, i = GPIO_CFG_HIGH; i >= GPIO_CFG_LOW; i--)
		id = (id << 1) + gpio_get_val(GPIO_CFG_PORT, i);

	/* Handle bug in Dev boards where ID came back as 0x1f.  Map to 0 */
	if (id == 0x1f)
		id = 0;

	/* save into scratchpad register */
	id      &= BIT_MASK_ONES(SCRATCH_BOARD_ID_SIZE);
	scratch  = gpio_get_scratchpad();
	scratch &= ~(BIT_MASK_ONES(SCRATCH_BOARD_ID_SIZE) 
                 <<	SCRATCH_BOARD_ID_POS);
	scratch |= (id << SCRATCH_BOARD_ID_POS);
	gpio_set_scratchpad(scratch);
	return id;
}	

static void load_cart_id(void)
{
	u32 scratch;
	u32 id = 0;
	int i;

	for(i = GPIO_CART_CFG_HIGH; i >= GPIO_CART_CFG_LOW; i--)
		gpio_configure_pin(GPIO_CART_CFG_PORT, i, GPIO_GPIOFN, 0, 0, 0);

	for(i = GPIO_CART_CFG_HIGH; i >= GPIO_CART_CFG_LOW; i--)
		id = (id<<1) + gpio_get_val(GPIO_CART_CFG_PORT, i);

	/* save into scratchpad register */
	id      &= BIT_MASK_ONES(SCRATCH_CART_ID_SIZE);
	scratch  = gpio_get_scratchpad();
	scratch &= ~(BIT_MASK_ONES(SCRATCH_CART_ID_SIZE) <<
			SCRATCH_CART_ID_POS);
	scratch |= (id<<SCRATCH_CART_ID_POS);
	gpio_set_scratchpad(scratch);
}

/* Supervise processing before waiting for connection to a USB host
 *   write a string to the serial port
 *   display a "Connect" screen
 *   set bookkeeping flags
 *   wait (forever) for connection to a USB host
 */
static void 
show_attention_needed_and_wait( const char * ps, 
                                enum scratch_request value,
                                int  board_id)
{
	serio_puts(ps);
	show_attention_needed ();
	display_init();
	set_boot_image(SCRATCH_BOOT_IMAGE_RECOVERY);
	set_request (value);
	load_boot_source(0, 1);
	wait_for_usb_host(board_id);  
                                // not expected to return
	die();                      // but just in case it does, shut down.
}

/*
 * main application
 */

void main(void)
{
	u32 image = 0;
	u32 kernel_nand_addr = 0;
	int board_id;
	u16 bitrate;
	struct nand_size_info baseInfo;
	const char *ram_cmdline;
	enum scratch_request request, request0;
	int panic;
	enum scratch_shutdown shutdown;
	enum scratch_power power;
	struct buttons_state buttons;
	int button_was_pressed = 0;
	u32 *params_buffer;
	struct display_module *display_info = NULL;
	// db_stopwatch_start("Before");

	/* Ping oscilloscope */
	/* Configure GPIOB_0 and GPIOB_1 for the cart LEDs */
	led_config ();
	LED1_OFF; LED2_ON;
	// gpio_configure_pin(GPIO_PORT_B, GPIO_PIN8, GPIO_GPIOFN, 1, 0, 1);
	// gpio_configure_pin(GPIO_PORT_B, GPIO_PIN8, GPIO_GPIOFN, 1, 0, 0);

	/* disable the USB controller */
	BIT_SET(REG16(LF1000_UDC_BASE+UDC_PCR), PCE);

	/* Work-around for Micron 512MB flash forgetting it's Bad-block marks */
	nand_wake_bbt (&baseInfo);
	serio_puts ("EBS="); serio_int (baseInfo.eb_size);
	serio_puts (","); serio_int (baseInfo.eb_shift);
	serio_puts (" Page="); serio_int (baseInfo.page_size);
	serio_puts (","); serio_int (baseInfo.page_shift);
	serio_puts (" P/E="); serio_int (baseInfo.pages_per_eb);
	serio_puts (" oobsize="); serio_int (baseInfo.oob_size);
	serio_puts (" eccbytes="); serio_int (baseInfo.ecc_bytes);
	serio_puts (" eccoffset="); serio_int (baseInfo.ecc_offset);
	serio_puts (" eccsteps="); serio_int (baseInfo.ecc_steps);
	serio_puts ("\n");

	ram_cmdline = probe_ram ();
	adc_init();
	power = load_power_state();
	board_id = load_board_id();
	display_backlight(board_id);
	clock_init();
	bitrate = db_init();
	load_boot_source(bitrate, 0);  // bitrate is non-zero if UART boot

	// The string is output, even when DEBUG is not defined
	serio_puts(EMERALD_BOOT_VERSION_STRING);	
	// Give a clue: dump the keep-alive register raw
	serio_int (gpio_get_scratchpad());
	serio_puts("\n");


#if !defined(CONFIG_LF1000_DISABLE_LOW_BATTERY_CHECK) && defined(LOW_BATTERY_NO_BOOT)
	/* now that backlight is on, see if we have enough battery to boot */
	if(gpio_get_val(LOW_BATT_PORT, LOW_BATT_PIN) == 0 && 
		ADC_TO_MV(adc_get_reading(LF1000_ADC_VBATSENSE)) < BOOT_MIN_MV){
		serio_puts("PANIC: battery voltage too low!\n");
		die();
	}
#endif /* !CONFIG_LF1000_DISABLE_LOW_BATTERY_CHECK && LOW_BATTERY_NO_BOOT */

	/* 5th priority: Requested boot type */
	request0 = request = get_request ();
	panic = get_panic ();
	switch (request0)
	{
	default:	db_puts ("Read REQUEST unknown value\n");
		request = SCRATCH_REQUEST_PLAY;
		/* Fall into _PLAY */
	case SCRATCH_REQUEST_PLAY:	db_puts ("Read REQUEST_PLAY\n");
		break;
	case SCRATCH_REQUEST_RETURN:	db_puts ("Read REQUEST_RETURN\n");
		break;
	case SCRATCH_REQUEST_UPDATE:	db_puts ("Read REQUEST_UPDATE\n");
		break;
	case SCRATCH_REQUEST_BATTERY:	db_puts ("Read REQUEST_BATTERY\n");
		break;
	case SCRATCH_REQUEST_UNCLEAN:	db_puts ("Read REQUEST_UNCLEAN\n");
		break;
	case SCRATCH_REQUEST_FAILED:	db_puts ("Read REQUEST_FAILED\n");
		break;
	case SCRATCH_REQUEST_TRAPDOOR:	db_puts ("Read REQUEST_TRAPDOOR\n");
		break;
	case SCRATCH_REQUEST_SHORT:	db_puts ("Read REQUEST_SHORT\n");
		break;
	}

	/* 4th priority: Test for batteries pulled */
	if (power == SCRATCH_POWER_FIRSTBOOT)
	{
		// Batteries were pulled: test and possibly reboot into play if no problem
		db_puts ("Read POWER_FIRSTBOOT\n");
		request = SCRATCH_REQUEST_BATTERY;
	}

	/* 3rd priority: Test for dirty shutdown */
	shutdown = load_shutdown_state ();
	if (shutdown == SCRATCH_SHUTDOWN_DIRTY)
	{
		// Boot into recover: test and possibly reboot into play if no problem
		db_puts ("Read SHUTDOWN_DIRTY\n");
		request = SCRATCH_REQUEST_UNCLEAN;
	}

	/* 2nd priority: Test for failed I/O */

	/* 1st priority: Button press */
	/*
	 * Signal "trapdoor" if Button A pressed
	 * Signal "short-circuit" if Button B pressed
	 * Note that GPIO pin is zero when pressed
	 */

	buttons_get_state(board_id, &buttons);

	/* detect and initialize the LCD display module and DPC */
	display_info = display_setup();

	/* construct the command line */
	strcpy(cmdline_buffer, CMDLINE_BASE);
#ifdef DEBUG_NAND_ID
	strcat (cmdline_buffer, mybuf);
#endif
	/* tack on ram size */
	strcat (cmdline_buffer, ram_cmdline);

	/* tack on display info, if any */
	if (display_info) {
		strcat(cmdline_buffer, "screen_module=");
		strcat(cmdline_buffer, display_info->name);
		strcat(cmdline_buffer, " ");
	}

	/* Rootfs parts */
	strcat (cmdline_buffer, CMDLINE_RFS);

	/* If enabled, check button sequences and do kernel bad-block loading
	 * tests as requested. */
	do_kernel_bad_block_test(&buttons);

	if (buttons.ls && buttons.rs && buttons.hint)
	{
		// USB boot force
		show_attention_needed_and_wait("Buttons LS+RS+Hint held: USB boot\n",
                                        SCRATCH_REQUEST_TRAPDOOR, board_id);
		// Never returns
	}
	else if (request0 == SCRATCH_REQUEST_UPDATE)
	{
		// USB boot force
		show_attention_needed_and_wait("Request=UPDATE: USB boot\n",
                                        SCRATCH_REQUEST_TRAPDOOR, board_id);
		// Never returns
	}
	else if (buttons.a && buttons.b)
	{
		if (buttons.p)
		{
			// Short-circuit boot Trapdoor was pressed
			button_was_pressed = 1;
			request = SCRATCH_REQUEST_SHORT;
			db_puts("Buttons A+B+Pause: short-circuit boot\n");
		}
		else
		{
			// Manufacturing Test Trapdoor was pressed
			button_was_pressed = 1;
			request = SCRATCH_REQUEST_TRAPDOOR;
			db_puts("Buttons A+B: manufacturing test trapdoor\n");
		}
	}
	else if (panic > 1)
	{
		// USB boot force
		serio_puts ("Kernel Panic=");
		serio_hex (panic);
		show_attention_needed_and_wait(": USB boot\n",
				       SCRATCH_REQUEST_TRAPDOOR, board_id);
		// Never returns
	}

	/* Set up the kernel command line */

	/* Last minute things before launching kernel */
	set_boot_image(SCRATCH_BOOT_IMAGE_PLAY);
	set_request (request);

	kernel_nand_addr = BOOT0_ADDR(baseInfo.eb_size);

	LED1_ON; LED2_OFF;
	// db_stopwatch_stop();
	// db_stopwatch_start("Read NAND");

	/* Read in NAND until we're done */
	cbf_init ();
	u32 offset = kernel_nand_addr;
	int status = 1;
	u8 *b = (u8*)nandio_buffer;
	db_puts("b=");
	db_int ((unsigned int)b);
	db_puts("\n");

	while (status == 1)
	{
		/* Skip bad blocks */
		while(baseInfo.nand_check_block(offset, &baseInfo) != 0) {
			serio_puts("Kernel: skipped bad block at ");
			serio_int(offset);
			serio_puts("\n");
			offset += baseInfo.eb_size;
		}
		/* fetch one block of data */	
		if (0 > (baseInfo.nand_read)((void *)b, offset, baseInfo.eb_size, &baseInfo)) {
            /* if there was a read error, display the Connect screen
             * and wait for connection to a USB host
             */
            show_attention_needed_and_wait("Error reading kernel from NAND\n",
                                            SCRATCH_REQUEST_FAILED, board_id);
        }
		offset += baseInfo.eb_size;
		/* Process it */
		status = cbf_process_chunk (b, baseInfo.eb_size, &b);
	}
	/* Check for an error */
	if (status == -1)
	{
		show_attention_needed_and_wait("CBF: NAND Boot failure: cbf_process_chunk failed "
                                       "during kernel load\n",
                                        SCRATCH_REQUEST_FAILED, board_id);
	}

	db_puts (cmdline_buffer); db_puts ("\n");
	params_buffer = (u32 *)(calc_PARAMS_ADDRESS());

	image = load_kernel(cmdline_buffer, params_buffer);
	if(image == 0) {
        show_attention_needed_and_wait("CBF: NAND Boot failure: load_kernel returned entry point=0\n",
                                        SCRATCH_REQUEST_FAILED, board_id);
	}

	// db_stopwatch_stop();
	// db_stopwatch_start("Read H&S");

	/* About to load H&S screen from NAND */
	LED1_OFF; LED2_OFF;

	// gpio_configure_pin(GPIO_PORT_B, GPIO_PIN8, GPIO_GPIOFN, 1, 0, 1); 
	// gpio_configure_pin(GPIO_PORT_B, GPIO_PIN8, GPIO_GPIOFN, 1, 0, 0); 

	// Show first screen
#if !defined(CONFIG_LF1000_DISABLE_LOW_BATTERY_CHECK) && defined(LOW_BATTERY_SHOW_SCREEN)
	// Check battery voltage and display LOW BATTERY screen if too low
#ifdef DEBUG
	int v=ADC_TO_MV(adc_get_reading(LF1000_ADC_VBATSENSE));
	db_puts("Voltage=");
	db_16(v);
	db_puts("\n");
#endif
	if(gpio_get_val(LOW_BATT_PORT, LOW_BATT_PIN) == 0 && 
		ADC_TO_MV(adc_get_reading(LF1000_ADC_VBATSENSE)) < BOOT_MIN_MV)
	{
		show_low_battery ();
		display_init();
	}
	else
	{
#endif /* !CONFIG_LF1000_DISABLE_LOW_BATTERY_CHECK && LOW_BATTERY_SHOW_SCREEN */
		// Health & Safety in raw mtd1 partition as rle
		u32 *hns_buffer = nandio_buffer;
		int hns_size = (1+(HNS_SIZE >> baseInfo.page_shift))*baseInfo.page_size;

		offset = BOOT_FLAGS_ADDR(baseInfo.eb_size);
		while (offset < BOOT0_ADDR(baseInfo.eb_size))
		{
			if (baseInfo.nand_check_block(offset, &baseInfo) == 0) /* nand eraseblock is good */
			{
				if (0 > (baseInfo.nand_read)((void*) hns_buffer, offset, hns_size, &baseInfo))
				{   /* good block, but uncorrectable error */
					show_attention_needed_and_wait(
						"Uncorrectable error reading H&S screen from nand\n",
						SCRATCH_REQUEST_FAILED,
						board_id);
				}
				break;
			}
			else {  // bad block; step to the next block
				serio_puts("H&S: skipped bad block at ");
				serio_int(offset);
				serio_puts("\n");
				offset += baseInfo.eb_size;
			}
		}
		if (offset >= BOOT0_ADDR(baseInfo.eb_size)) /* all blocks bad */
		{
			show_attention_needed_and_wait(
				"Error reading H&S screen from NAND: all blocks bad\n",
				SCRATCH_REQUEST_FAILED, board_id);
		}
		rle_decode (hns_buffer, (void *)get_frame_buffer_addr());
		display_init();
#if !defined(CONFIG_LF1000_DISABLE_LOW_BATTERY_CHECK) && defined(LOW_BATTERY_SHOW_SCREEN)
	}
#endif /* !CONFIG_LF1000_DISABLE_LOW_BATTERY_CHECK && LOW_BATTERY_SHOW_SCREEN */

	load_cart_id();

	/* Done loading H&S screen from NAND */
	LED1_ON; LED2_ON;

	/* Ping oscilloscope -- leave high for kernel */
	// gpio_configure_pin(GPIO_PORT_B, GPIO_PIN8, GPIO_GPIOFN, 1, 0, 1);
	// gpio_configure_pin(GPIO_PORT_B, GPIO_PIN8, GPIO_GPIOFN, 1, 0, 0);

	// db_stopwatch_stop();

	db_puts("Starting the kernel...\n");
	db_puts("...jumping to ");
	db_int(image);
	db_puts("\n");

	cleanup_for_linux();
	/* jump to image (void, architecture ID, atags pointer) */
	((void(*)(int r0, int r1, unsigned int r2))image)
		(0, MACH_TYPE_DIDJ, (unsigned int)params_buffer);

	/* never get here! */
	die();
}


