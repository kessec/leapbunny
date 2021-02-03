/* main.c -- Emerald Platform Bootloader
 *
 * Copyright 2007-2011 LeapFrog Enterprises Inc.
 *
 * Robert Dowling <rdowling@leapfrog.com>
 * Scott Esters <sesters@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <board.h>
#include <common.h>
#include <base.h>
#include <setup.h>
#include <nand.h>
#include <nand_controller.h>
#include <gpio.h>
#include <gpio_hal.h>
#include <gpio_map.h>
#include <stdint.h>
#include <adc.h>
#include <disk.h>
#include <mmc.h>
#include <string.h>
#include <debug.h>
#include <cbf.h>
#include <display.h>
#include <global.h>
#include <clock.h>
#include <ram.h>
#include <rle.h>
#include <screens.h>
#include <versions.h>
#include <led_config.h>
#include <buttons.h>
#include <bootUsb.h>
#include <lfp100.h>
#include <i2c.h>

#if HNS_SIZE == 0
#error "HNS_SIZE not known"
#endif

/* static buffers: 
 * These are automatically put into hi or lo memory (thus, into real RAM)
 * by compiler runtime code, as their location is relative to the program
 * counter.
*/
static u32 io_buffer[WORST_EB_SIZE/sizeof(u32)];
#define IO_BUFFER_SIZE (WORST_EB_SIZE/sizeof(u32))

#ifdef DEBUG_NAND_ID
extern char mybuf[1024];
#endif

/* USB controller */
#define UDC_PCR		0x52	/* PCR register offset */
#define PCE		0	/* not-enable bit */

u32 calc_PARAMS_ADDRESS();

/*
 * Die in case of unrecoverable error.  On LF1000, we pull the power off. 
 * Otherwise just lock up. 
 */
void die(void)
{
	global_var *gptr = get_global();

	db_puts("die()\n");

	/* enable access to Alive GPIO */
	REG32(ALIVE_BASE+ALIVEPWRGATEREG) = 1;
	/* pull VDDPWRON low by resetting the flip-flop */
	BIT_CLR(REG32(ALIVE_BASE+ALIVEGPIOSETREG), VDDPWRONSET);
	BIT_SET(REG32(ALIVE_BASE+ALIVEGPIORSTREG), VDDPWRONSET);
	/* reset flip-flop to latch in */
	REG32(ALIVE_BASE+ALIVEGPIOSETREG) = 0;
	REG32(ALIVE_BASE+ALIVEGPIORSTREG) = 0;

	if (gptr->use_lfp100) {
		lfp100_power_off();
	}

	/* power should be off now... */

	/* Turn screen off */
	gpio_configure_pin(GPIO_PORT_A, GPIO_PIN30, GPIO_GPIOFN, 1, 0, 0);

	while(1);
}

/*
 * Clean up before booting Linux on the ARM926: turn off instruction cache and 
 * make sure data cache doesn't contain any stale data. 
 */
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

	db_puts("build_params at ");
	db_int((unsigned int)params);

	/* set up the core tag */

	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size(tag_core);
	params->u.core.flags = 0;
	params->u.core.pagesize = 0;
	params->u.core.rootdev = 0;
	params = tag_next(params);

	/* set up the kernel command line tag */

	if (cmdline == 0)
		goto END_TAG;
	
	/* eat leading spaces */
	for (p = cmdline; *p == ' '; p++);

	if (*p == '\0')
		goto END_TAG;

	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size = (sizeof(struct tag_header) + strlen(p) + 1 + 4)>>2;
	strcpy(params->u.cmdline.cmdline, p);
	params = tag_next(params);

	/* set up the end tag */
END_TAG:
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;

	db_puts("-");
	db_int((unsigned int)tag_next(params));
	db_puts("\n");
}

#ifdef RAMDISK
extern u8 __ramdisk_nand__[];
extern u8 __ramdisk_nor__[];
u32 GetRamDiskAddr() {
	if (IS_CLR(REG32(MCU_S_BASE + NFCONTROL), NFBOOTENB))
		/* NOR BOOT, shadow disabled */
		return (u32)__ramdisk_nor__;
	else
		/* NAND BOOT, shadow enabled */
		return (u32)__ramdisk_nand__;
}
#endif

u32 calc_PARAMS_ADDRESS()
{
	extern char my_kernel_params[0x800]; /* See params.c */

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
	if (cbf_get_jump_address(&entry_point)) {
		db_puts("Failed to get kernel entry_point\n");
		return 0;
	}

	db_puts("entry_point=");
	db_int((unsigned int)entry_point);
	db_putchar('\n');

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
	scratch &= ~(BIT_MASK_ONES(SCRATCH_BOOT_IMAGE_SIZE) 
			<< SCRATCH_BOOT_IMAGE_POS);
	scratch |= ((id & BIT_MASK_ONES(SCRATCH_BOOT_IMAGE_SIZE))
			<< SCRATCH_BOOT_IMAGE_POS);
	gpio_set_scratchpad(scratch);
}

/* Get boot request */
static enum scratch_request get_request(void)
{
	return (gpio_get_scratchpad() >> SCRATCH_REQUEST_POS) &
		BIT_MASK_ONES(SCRATCH_REQUEST_SIZE);
}

/* Set boot request */
static void set_request(enum scratch_request value)
{
	u32 scratch = gpio_get_scratchpad();
	scratch &= ~(BIT_MASK_ONES(SCRATCH_REQUEST_SIZE) 
			<< SCRATCH_REQUEST_POS);
	scratch |= (value << SCRATCH_REQUEST_POS);
	gpio_set_scratchpad(scratch);
}

/* Get panic */
static int get_panic(void)
{
	return (gpio_get_scratchpad() >> SCRATCH_PANIC_POS) &
		BIT_MASK_ONES(SCRATCH_PANIC_SIZE);
}

/*
 * Retrive old shutdown reason, set shutdown bit to 'DIRTY'
 */
static enum scratch_shutdown load_shutdown_state(void)
{
	enum scratch_shutdown id;
	u32 scratch;

	scratch = gpio_get_scratchpad();

	/* get last shutdown status */
	id = (scratch >> SCRATCH_SHUTDOWN_POS) &
		BIT_MASK_ONES(SCRATCH_SHUTDOWN_SIZE);

	/* reset shutdown bit to 'DIRTY', Linux sets to 'CLEAN' at shutdown */
	scratch &= ~(BIT_MASK_ONES(SCRATCH_SHUTDOWN_SIZE)<<
			SCRATCH_SHUTDOWN_POS);
	scratch |= (SCRATCH_SHUTDOWN_DIRTY << SCRATCH_SHUTDOWN_POS);
	gpio_set_scratchpad(scratch);

	return id;
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

	boot = IS_SET(REG32(MCU_S_BASE+NFCONTROL),NFBOOTENB);
	if (usb)
		id = SCRATCH_BOOT_SOURCE_USB;
	else if (!boot)
		id = SCRATCH_BOOT_SOURCE_NOR;
	else if (bitrate) /* check UART Bit Rate Divisor */
		id = SCRATCH_BOOT_SOURCE_UART; /* UART boot sets divisor */
	else
		id = SCRATCH_BOOT_SOURCE_NAND;

	/* set boot source */
	scratch = gpio_get_scratchpad();
	scratch &= ~(BIT_MASK_ONES(SCRATCH_BOOT_SOURCE_SIZE)<<
			SCRATCH_BOOT_SOURCE_POS);
	scratch |= id<<SCRATCH_BOOT_SOURCE_POS;
	gpio_set_scratchpad(scratch);

	return id;
}

static int load_board_id(void)
{
	u32 id, scratch;
	int i;

	/* make pins inputs */
	for (i = GPIO_CFG_HIGH; i >= GPIO_CFG_LOW; i--)
		gpio_configure_pin(GPIO_CFG_PORT, i, GPIO_GPIOFN, 0, 0, 0);

	/* read pins */
	for (id = 0, i = GPIO_CFG_HIGH; i >= GPIO_CFG_LOW; i--)
		id = (id<<1) + gpio_get_val(GPIO_CFG_PORT, i);

	/* Handle bug in Dev boards where ID came back as 0x1f.  Map to 0 */
	if (id == 0x1f)
		id = 0;

	/* save into scratchpad register */
	id &= BIT_MASK_ONES(SCRATCH_BOARD_ID_SIZE);
	scratch = gpio_get_scratchpad();
	scratch &= ~(BIT_MASK_ONES(SCRATCH_BOARD_ID_SIZE)<<
			SCRATCH_BOARD_ID_POS);
	scratch |= id<<SCRATCH_BOARD_ID_POS;
	gpio_set_scratchpad(scratch);
	return id;
}	

static void load_cart_id(void)
{
	u32 scratch;
	u32 id = 0;
	int i;

	for (i = GPIO_CART_CFG_HIGH; i >= GPIO_CART_CFG_LOW; i--)
		gpio_configure_pin(GPIO_CART_CFG_PORT, i, GPIO_GPIOFN,
				0, 0, 0);

	for (i = GPIO_CART_CFG_HIGH; i >= GPIO_CART_CFG_LOW; i--)
		id = (id<<1) + gpio_get_val(GPIO_CART_CFG_PORT, i);

	/* save into scratchpad register */
	id &= BIT_MASK_ONES(SCRATCH_CART_ID_SIZE);
	scratch = gpio_get_scratchpad();
	scratch &= ~(BIT_MASK_ONES(SCRATCH_CART_ID_SIZE)<<SCRATCH_CART_ID_POS);
	scratch |= id<<SCRATCH_CART_ID_POS;
	gpio_set_scratchpad(scratch);
}

/* Supervise processing before waiting for connection to a USB host
 *   write a string to the serial port
 *   display a "Connect" screen
 *   set bookkeeping flags
 *   wait (forever) for connection to a USB host
 */
static void show_attention_needed_and_wait(const char * ps,
		enum scratch_request value)
{
	global_var * gptr = get_global();

	serio_puts(ps);
	show_attention_needed();
	display_init();
	set_boot_image(SCRATCH_BOOT_IMAGE_RECOVERY);
	set_request(value);
	load_boot_source(0, 1);

	/* construct a safe command line */
	strcpy(gptr->cmdline_buffer, CMDLINE_BASE);
	/* tack on ram size */
	strcat(gptr->cmdline_buffer, probe_ram());
	/* tack on display info, if any */
	if (gptr->display_info) {
		strcat(gptr->cmdline_buffer, "screen_module=");
		strcat(gptr->cmdline_buffer, gptr->display_info->name);
		strcat(gptr->cmdline_buffer, " ");
	}
	serio_puts(gptr->cmdline_buffer);
	serio_puts("\n");
	wait_for_usb_host(gptr->display_info); /* doesn't return */
	
	die();
}

#ifdef DEBUG
void dbg_show_voltage_level(void)
{
	int v = ADC_TO_MV(adc_get_reading(BOARD_ADC_VBATSENSE));
	db_puts("Voltage=");
	db_16(v);
	db_puts("\n");
}
#else
#define dbg_show_voltage_level()
#endif

static int is_battery(void)
{
	global_var * gptr = get_global();

	int ret = 0;

	db_puts("is_battery user_lfp100=");db_int(gptr->use_lfp100);
	db_puts("board_id=");db_int(gptr->board_id); db_putchar('\n');
	if (gptr->use_lfp100) {
		if  (lfp100_get_power_source() == BATTERY)
			ret = 1;
	} else {
		switch(gptr->board_id) {
		case LF1000_BOARD_DEV:
			if (gpio_get_val(DEV_EXT_POWER_PORT,
					 DEV_EXT_POWER_PIN) == 0)
				ret = 1;
			break;
		case LF1000_BOARD_DIDJ:
		case LF1000_BOARD_DIDJ_09:
			if (gpio_get_val(DIDJ_EXT_POWER_PORT,
					DIDJ_EXT_POWER_PIN) == 0)
				ret = 1;
			break;
		case LF1000_BOARD_ACORN:
			if (gpio_get_val(ACORN_EXT_POWER_PORT,
					ACORN_EXT_POWER_PIN) == 0)
				ret = 1;
			break;
		case LF1000_BOARD_EMERALD_POP:
		case LF1000_BOARD_EMERALD_NOTV_NOCAP:
		case LF1000_BOARD_EMERALD_TV_NOCAP:
		case LF1000_BOARD_EMERALD_NOTV_CAP:
		case LF1000_BOARD_EMERALD_SAMSUNG:
			if (gpio_get_val(EMERALD_EXT_POWER_PORT,
					EMERALD_EXT_POWER_PIN) == 0)
				ret = 1;
			break;
		case LF1000_BOARD_K2:
			if (gpio_get_val(K2_EXT_POWER_PORT,
					K2_EXT_POWER_PIN) == 0)
				ret = 1;
			break;
		case LF1000_BOARD_MADRID:
		case LF1000_BOARD_MADRID_POP:
			if (gpio_get_val(MADRID_EXT_POWER_PORT,
					MADRID_EXT_POWER_PIN) == 0)
				ret = 1;
			break;
		default:
			db_puts("unknown board type\n");
			break;
		}
	}
	db_puts("is_battery ret=");db_int(ret);db_putchar('\n');
	return ret;
}

static int isMadrid( int board_id) 
{
	int ret;

	switch (board_id) {
	case LF1000_BOARD_MADRID:
	case LF1000_BOARD_MADRID_POP:
	case LF1000_BOARD_MADRID_LFP100:
		ret = 1;
		break;
	default:
		ret = 0;
		break;
	}
	return ret;
}

#define PWR_LAMP_PORT_MADRID	GPIO_PORT_C
#define PWR_LAMP_PIN_MADRID	GPIO_PIN13

void turn_on_pwr_lamp(void)
{
	global_var * gptr = get_global();
	
	switch(gptr->board_id)
	{
		case LF1000_BOARD_MADRID:
		case LF1000_BOARD_MADRID_POP:
		case LF1000_BOARD_MADRID_LFP100:
			/* Turn on power LED on Madrid */
			gpio_configure_pin(PWR_LAMP_PORT_MADRID, PWR_LAMP_PIN_MADRID, GPIO_GPIOFN, 1, 0, 1);
			break;
	}
}

#define PARTITION_SCREENS	0
#define PARTITION_KERNEL	1

/* Load everything that we need from SD and display a splash screen.  Return
 * the address of the kernel image in memory to jump to. */
u32 load_sd(u32 sdhc_base, struct disk_partition *partitions, u32 *buf,
		u32 *params_buffer)
{
	struct disk_partition *p;
	unsigned int sector;
	unsigned int hns_sectors;
	int kernel_sectors;
	int status;
	u8 *b = (u8 *)buf;
	u32 *dest;
	u32 image;
	void * pSplash;
	global_var * gptr = get_global();


	/* read the H&S screen into memory */

	p = &partitions[PARTITION_SCREENS];
	if (p->type != DISK_TYPE_LINUX)
		goto out_sd_hns;

	cbf_init();		// takes no appreciable time
	dest = buf;
	sector = p->start;
		if (mmc_read_sector(sdhc_base, sector, dest))
		{
			serio_puts ("Error reading H&S screen from SD\n");
			goto out_sd_hns;
		}
	status = cbf_process_chunk(b, DISK_SECTOR_SIZE, &b);
	if( status < 0 )
		goto out_sd_hns;
	//FW_DEBUG_2_ON;

	hns_sectors = cbf_get_data_size();
	if (hns_sectors <= 0) 
		goto out_sd_hns;

	hns_sectors = (hns_sectors + DISK_SECTOR_SIZE - 1)/DISK_SECTOR_SIZE;
	if (hns_sectors > p->length)	 /* if cbf pkg size bigger than partition */
		goto out_sd_hns;
	//FW_DEBUG_1_ON;
	/* Read the rest of the H&S screen */
	if(mmc_read_multiblock(sdhc_base, p->start + 1, hns_sectors - 1, (void*)b))
		goto out_sd_hns;
	//FW_DEBUG_2_ON;

	//FW_DEBUG_2_ON;				// ~85msec for cbf_process_chunck()
	status = cbf_process_chunk(b, DISK_SECTOR_SIZE * (hns_sectors - 1), &b);
	if( status < 0 )
		goto out_sd_hns;
	//FW_DEBUG_2_ON;
	
		/* Here we assume the jump address is the same as the load address */
	if (cbf_get_jump_address(&pSplash) != 0)
		goto out_sd_hns;

	/* display the screen */
	rle_decode((u32 *)pSplash, (void *)get_frame_buffer_addr());
	goto init_disp;


out_sd_hns:
	fill_fb(0xFFFFFFFF, (void *)get_frame_buffer_addr());

init_disp:
	b = (u8 *)buf;

	display_init();		// takes no appreciable time
	
	cbf_init();		// takes no appreciable time

	/* read the 'kernel' partition into memory */

	p = &partitions[PARTITION_KERNEL];
	if (p->type != DISK_TYPE_LINUX)
	{
		goto out_sd_kernel;
	}
	
	//FW_DEBUG_1_ON;
	/* Read the cbf header in */
	// the mmc_read_sector() call takes almost no time
	if (mmc_read_sector(sdhc_base, p->start, (void *)b))
		goto out_sd_kernel;
	//FW_DEBUG_2_ON;
	//FW_DEBUG_1_ON;
	// the cbf_process_chunk() call takes almost no time
	status = cbf_process_chunk(b, DISK_SECTOR_SIZE, &b);
	if( status < 0 )
		goto out_sd_kernel_cbf;
	//FW_DEBUG_2_ON;
	
	kernel_sectors = cbf_get_data_size();
	if (kernel_sectors <= 0) 
		goto out_sd_kernel_cbf_size;

	kernel_sectors = (kernel_sectors + DISK_SECTOR_SIZE - 1)/DISK_SECTOR_SIZE;
	if (kernel_sectors > p->length)	 /* if cbf pkg size bigger than partition */
		goto out_sd_kernel_cbf_size; 
	FW_DEBUG_1_ON;
	/* Read the rest of the kernel */	// ~365msec for mmc_read_multiblock()
	if(mmc_read_multiblock(sdhc_base, p->start + 1, kernel_sectors - 1, (void*)b))
		goto out_sd_kernel;		
	//FW_DEBUG_2_ON;

	FW_DEBUG_2_ON;				// ~85msec for cbf_process_chunck()
	status = cbf_process_chunk(b, DISK_SECTOR_SIZE * (p->length - 1), &b);
	if( status < 0 )
		goto out_sd_kernel_cbf;
	//FW_DEBUG_2_ON;
	
	// the load_kernel() call takes almost no time
	image = load_kernel(gptr->cmdline_buffer, params_buffer);
	if (image == 0)
		goto out_sd_load_kernel;
	/* done loading kernel */

	/* check the battery, show low battery warning if too low */
	dbg_show_voltage_level();

	if (is_battery() &&
	    ADC_TO_MV(adc_get_reading(BOARD_ADC_VBATSENSE)) < BOOT_LOW_BATT) {
		show_low_battery();
		display_init();
		return image;
	}

	mmc_exit(sdhc_base);
	return image;

	/* failures: these show a "connect" screen and do not return */
out_sd_kernel_cbf:
	serio_puts ("CBF: SD Boot failure: cbf_process_chunk failed during kernel load\n");
	goto out;
out_sd_kernel_cbf_size:
	serio_puts ("CBF: SD Boot failure: bad cbf size\n");
	goto out;
out_sd_kernel:
	serio_puts ("CBF: SD Boot failure: Error reading kernel from SD\n");
	goto out;
out_sd_load_kernel:
	serio_puts ("CBF: SD Boot failure: load_kernel returned entry point=0\n");
out:
	mmc_exit(sdhc_base);
	return 0;
}

u32 load_nand(u8 *b, u32 *params_buffer)
{
	u32 offset;
	int status = 1;
	int ret;
	u32 *hns_buffer = (u32 *)b;
	int hns_size;
	u32 image;
	int any_good=0, tried=0;
	global_var * gptr = get_global();
	u32 kernel_nand_addr = BOOT0_ADDR(gptr->baseInfo.eb_size);

	cbf_init();
	offset = kernel_nand_addr;

	while (status == 1) {
		/* Skip bad blocks */
		while (gptr->baseInfo.nand_check_block(offset, &gptr->baseInfo) != 0) {
			serio_puts("Kernel: skipped bad block at ");
			serio_int(offset);
			serio_puts("\n");
			offset += gptr->baseInfo.eb_size;

#define MAX_BAD_INITIAL_BLOCKS_IN_KERNEL	16
			if (++tried > MAX_BAD_INITIAL_BLOCKS_IN_KERNEL && !any_good)
			{
				/* Never found a good block in first N... Give up */
				serio_puts ("Too many bad initial blocks in kernel area of NAND\n");
				return 0;
			}
		}

		/* fetch one block of data */
		ret = gptr->baseInfo.nand_read((void *)b, offset,
			gptr->baseInfo.eb_size, &gptr->baseInfo);
		/* if there was a read error, display the Connect screen
		 * and wait for connection to a USB host */
		if (ret < 0) {
			serio_puts ("Error reading kernel from NAND\n");
			return 0;
		}
		any_good=1;
		offset += gptr->baseInfo.eb_size;
		/* Process it */
		status = cbf_process_chunk(b, gptr->baseInfo.eb_size, &b);
	}

	/* Check for an error */
	if (status == -1) {
		serio_puts ("CBF: NAND Boot failure: cbf_process_chunk failed during kernel load\n");
		return 0;
	}

	image = load_kernel(gptr->cmdline_buffer, params_buffer);
	if (image == 0) {
		serio_puts ("CBF: NAND Boot failure: load_kernel returned entry point=0\n");
		return 0;
	}

	/* About to load H&S screen from NAND */

	// Show first screen
	// Check battery voltage and display LOW BATTERY screen if too low
	dbg_show_voltage_level();

	if (is_battery() &&
	    ADC_TO_MV(adc_get_reading(BOARD_ADC_VBATSENSE)) < BOOT_LOW_BATT) {
		show_low_battery();
		display_init();
		return image;
	}

	/* Health & Safety in raw mtd1 partition as rle */
	hns_size = (1+(HNS_SIZE >> gptr->baseInfo.page_shift))
		*gptr->baseInfo.page_size;

	offset = BOOT_FLAGS_ADDR(gptr->baseInfo.eb_size);
	while (offset < BOOT0_ADDR(gptr->baseInfo.eb_size)) {
		/* nand eraseblock is good */
		if (gptr->baseInfo.nand_check_block(offset, &gptr->baseInfo) == 0) {
			ret = gptr->baseInfo.nand_read((void *)hns_buffer, offset,
					hns_size, &gptr->baseInfo);
			/* good block, but uncorrectable error */
			if (ret < 0) { 
				serio_puts ("Uncorrectable error reading H&S screen from nand\n");
				return 0;
			}
			break;
		} else { /* bad block; step to the next block */
			serio_puts("H&S: skipped bad block at ");
			serio_int(offset);
			serio_putchar('\n');
			offset += gptr->baseInfo.eb_size;
		}
	}

	if (offset >= BOOT0_ADDR(gptr->baseInfo.eb_size)) { /* all blocks bad */
		serio_puts ("Error reading H&S screen from NAND: all blocks bad\n");
	}
	else
	{
		rle_decode(hns_buffer, (void *)get_frame_buffer_addr());
	}
	display_init();
	return image;
}

static inline void disable_usb_controller(void)
{
	BIT_SET(REG16(UDC_BASE + UDC_PCR), PCE);
}

void init_vibrator (void)
{
	global_var * gptr = get_global();

	switch(gptr->board_id) {
	case LF1000_BOARD_MADRID:
	case LF1000_BOARD_MADRID_LFP100:
	case LF1000_BOARD_MADRID_POP:
		// Low is off
		gpio_configure_pin(GPIO_PORT_A, GPIO_PIN30,
			GPIO_GPIOFN, 1, 0, 0);
		break;
	default:
		break;
	}
}

void search_for_nand(void)
{
	global_var * gptr = get_global();

	serio_puts("Search for NAND: ");
	gptr->found_nand = !nand_wake_bbt(&(gptr->baseInfo));
	if (gptr->found_nand)
	{
		serio_puts(" EBS="); serio_int (gptr->baseInfo.eb_size);
		serio_puts(","); serio_int (gptr->baseInfo.eb_shift);
		serio_puts(" Page="); serio_int (gptr->baseInfo.page_size);
		serio_puts(","); serio_int (gptr->baseInfo.page_shift);
		serio_puts(" P/E="); serio_int (gptr->baseInfo.pages_per_eb);
		serio_puts(" oobsize="); serio_int (gptr->baseInfo.oob_size);
		serio_puts(" eccbytes="); serio_int (gptr->baseInfo.ecc_bytes);
		serio_puts(" eccoffset="); serio_int (gptr->baseInfo.ecc_offset);
		serio_puts(" eccsteps="); serio_int (gptr->baseInfo.ecc_steps);
		serio_putchar('\n');
	}
	else
		serio_puts("No nand found\n");
}


void boot_nand(void)
{
	global_var * gptr = get_global();

	/* construct the command line */
	strcpy(gptr->cmdline_buffer, CMDLINE_BASE);
	strcat(gptr->cmdline_buffer, gptr->ram_cmdline);
	if (gptr->display_info) {
		strcat(gptr->cmdline_buffer, "screen_module=");
		strcat(gptr->cmdline_buffer, gptr->display_info->name);
		strcat(gptr->cmdline_buffer, " ");
	}
	serio_puts("\nboot_nand\n");

	strcat(gptr->cmdline_buffer, CMDLINE_RFS);
	/* Read in */
	gptr->image = load_nand((u8 *)io_buffer, gptr->params_buffer);
	if (gptr->image)
	{
		serio_puts ("NAND kernel passes CBF\n");
	}
	else
	{
		serio_puts ("NAND kernel fails CBF\n");
	}
}


void boot_sd1(void)
{
	struct disk_partition partitions[4];
	global_var * gptr = get_global();
	FW_DEBUG_0_ON;
	gptr->found_sd1 = disk_load_mbr(SDHC1_BASE, io_buffer, partitions) == 0;
	if (gptr->found_sd1) {
		serio_puts("SD1 found\n");
		/* construct the command line */
		strcpy(gptr->cmdline_buffer, CMDLINE_BASE);
		strcat(gptr->cmdline_buffer, gptr->ram_cmdline);
		if (gptr->display_info) {
			strcat(gptr->cmdline_buffer, "screen_module=");
			strcat(gptr->cmdline_buffer, gptr->display_info->name);
			strcat(gptr->cmdline_buffer, " ");
		}
		strcat(gptr->cmdline_buffer, CMDLINE_RFS_SD1);
		/* Read in */
		//FW_DEBUG_1_ON;	// disk_load_mbr() costs 350msec
		gptr->image = load_sd(SDHC1_BASE, partitions, io_buffer,
				gptr->params_buffer);
		if (gptr->image) {
			serio_puts ("SD1 kernel passes CBF\n");
		}
		else {
			serio_puts ("SD1 kernel fails CBF\n");
		}
	}
	else {
		serio_puts("No SD1 found\n");
	}
}


/*
 * main application
 */

void main(void)
{
	u16 bitrate;
	enum scratch_request request, request0;
	int panic;
	enum scratch_shutdown shutdown;
	enum scratch_power power;
	struct buttons_state buttons;
	global_var * gptr = get_global();	/* point at global data area */
#if 0	// Enable this if you want/need to use gpio for timing signals
		// Currently disabled to allow booting from ATAP NAND.
	fw_debug_led_config();
	// drive all debug lines low on the way in
	FW_DEBUG_0_OFF;
	FW_DEBUG_1_OFF;
	FW_DEBUG_2_OFF;
	FW_DEBUG_3_OFF;
#endif
	init_global();	/* zero out global data */

	serio_puts(EMERALD_BOOT_VERSION_STRING);	
	/* Give a clue: dump the keep-alive register raw */
	serio_int(gpio_get_scratchpad());
	serio_putchar('\n');

	gptr->board_id = load_board_id();
	
	turn_on_pwr_lamp();

	gptr->params_buffer = (u32 *)calc_PARAMS_ADDRESS();
	disable_usb_controller();

	gptr->ram_cmdline = probe_ram();

	adc_init();

	i2c_init(LFP100_I2C_CHANNEL);
	gptr->use_lfp100 = lfp100_have_chip();
	
	power = load_power_state();
	
	display_backlight();
	
	init_vibrator();

	serio_puts("clock_init: ");
	if (clock_init()) {
		serio_puts("(1) clocks changed\n");
	} else {
		serio_puts("(0) clocks NOT changed\n");
	}
	bitrate = db_init();
	load_boot_source(bitrate, 0); /* bitrate is non-zero if UART boot */

	/* detect and initialize the LCD display module and DPC */
	if (isMadrid(gptr->board_id)) {
		gptr->display_info = madrid_display_setup(&gptr->mad_disp_info);
	}
	else {
	gptr->display_info = display_setup();
	}

	/*
	 * now that backlight is on, if battery is too low
	 * shut down to stop the system from continuously
	 * resetting
	 */
	if (is_battery() &&
		ADC_TO_MV(adc_get_reading(BOARD_ADC_VBATSENSE)) < BOOT_MIN_MV) {
		serio_puts("PANIC: battery voltage too low!\n");
		die();
	}

	/* 5th priority: Requested boot type */
	request0 = request = get_request();
	panic = get_panic();
	switch (request0) {
		default:	
			db_puts ("Read REQUEST unknown value\n");
			request = SCRATCH_REQUEST_PLAY;
			/* Fall into _PLAY */
		case SCRATCH_REQUEST_PLAY:	
				db_puts ("Read REQUEST_PLAY\n");
				break;
		case SCRATCH_REQUEST_RETURN:	
				db_puts ("Read REQUEST_RETURN\n");
				break;
		case SCRATCH_REQUEST_UPDATE:	
				db_puts ("Read REQUEST_UPDATE\n");
				break;
		case SCRATCH_REQUEST_BATTERY:	
				db_puts ("Read REQUEST_BATTERY\n");
				break;
		case SCRATCH_REQUEST_UNCLEAN:	
				db_puts ("Read REQUEST_UNCLEAN\n");
				break;
		case SCRATCH_REQUEST_FAILED:	
				db_puts ("Read REQUEST_FAILED\n");
				break;
		case SCRATCH_REQUEST_TRAPDOOR:	
				db_puts ("Read REQUEST_TRAPDOOR\n");
				break;
		case SCRATCH_REQUEST_SHORT:	
				db_puts ("Read REQUEST_SHORT\n");
				break;
	}

	/* 4th priority: Test for batteries pulled */
	if (power == SCRATCH_POWER_FIRSTBOOT) {
		/* Batteries were pulled: test and possibly reboot into play
		 * if no problem */
		db_puts("Read POWER_FIRSTBOOT\n");
		request = SCRATCH_REQUEST_BATTERY;
	}

	/* 3rd priority: Test for dirty shutdown */
	shutdown = load_shutdown_state();
	if (shutdown == SCRATCH_SHUTDOWN_DIRTY) {
		/* Boot into recover: test and possibly reboot into play if no
		 * problem */
		db_puts("Read SHUTDOWN_DIRTY\n");
		request = SCRATCH_REQUEST_UNCLEAN;
	}

	/* 2nd priority: Test for failed I/O */

	/* 1st priority: Button press
	 *
	 * Signal "trapdoor" if Button A pressed
	 * Signal "short-circuit" if Button B pressed
	 * Note that GPIO pin is zero when pressed
	 */

	buttons_get_state(&buttons);
	if (buttons.ls && buttons.rs && buttons.hint) {
		show_attention_needed_and_wait(
				"Buttons LS+RS+Hint held: USB boot\n",
				SCRATCH_REQUEST_TRAPDOOR);
		/* Never returns */
	} else if (request0 == SCRATCH_REQUEST_UPDATE) {
		show_attention_needed_and_wait("Request=UPDATE: USB boot\n",
				SCRATCH_REQUEST_TRAPDOOR);
		/* Never returns */
	} else if (buttons.a && buttons.b) {
		if (buttons.p) {
			request = SCRATCH_REQUEST_SHORT;
			db_puts("Buttons A+B+Pause: short-circuit boot\n");
		} else {
			request = SCRATCH_REQUEST_TRAPDOOR;
			db_puts("Buttons A+B: manufacturing test trapdoor\n");
		}
	} else if (panic > 1) { /* USB boot force */
		serio_puts("Kernel Panic=");
		serio_hex(panic);
		show_attention_needed_and_wait(": USB boot\n",
				       SCRATCH_REQUEST_TRAPDOOR);
		/* Never returns */
	}

	/*
	 * Look for a bootable media device and
	 * if found, bring in a kernel checksum
	 */
		// If booted from nand or if not a madrid board, search_for_nand
	if (    IS_SET(REG32(MCU_S_BASE+NFCONTROL),NFBOOTENB)
		|| !isMadrid(gptr->board_id))
	{
		if ( !gptr->image ) {
			search_for_nand();
			if ( gptr->found_nand )
				boot_nand();
		}
	}
#if 0	// original
	if ( !gptr->image ) 
#else	
	else	// new: call boot_sd1() only if (isMadrid() && not a NAND boot) 
#endif
	{
		boot_sd1();
	}

	/* Nothing found? */
	if (gptr->image == 0)
	{
		show_attention_needed_and_wait(
			"No bootable media found; USB Boot\n",
			SCRATCH_REQUEST_FAILED);
	}

	/* Last minute things before launching kernel */
	set_boot_image(SCRATCH_BOOT_IMAGE_PLAY);
	set_request(request);

	load_cart_id();

	db_puts("Starting the kernel...jumping to \n");
	db_int(gptr->image);
	db_putchar('\n');

	cleanup_for_linux();

#if 1
	// drive all debug lines low on the way out, shows the end of main()
	FW_DEBUG_0_OFF; 
	FW_DEBUG_1_OFF;
	FW_DEBUG_2_OFF;	
	FW_DEBUG_3_ON;
#endif
	/* jump to image (void, architecture ID, atags pointer) */
	((void(*)(int r0, int r1, unsigned int r2))gptr->image)
		(0, MACH_TYPE, (unsigned int)gptr->params_buffer);

	/* never get here! */
	die();
}
