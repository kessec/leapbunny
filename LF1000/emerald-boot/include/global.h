/* global.h -- global variable definitions
 *
 * Copyright 2011 LeapFrog Enterprises Inc.
 *
 * Scott Esters <sesters@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef GLOBAL_H

#include <display.h>
#include <nand.h>

#define CMDLINE_LEN	4096

typedef struct global_var {
	int board_id;				/* hardware board ID */
	int use_lfp100;				/* using LFP100 chip */

	int found_nand;				/* found NAND boot device */
	int found_sd0;				/* found SD0 boot device */
	int found_sd1;				/* found SD1 boot device */
	unsigned int image;			/* image entry address */
	char * ram_cmdline;			/* size of memory */
	u32 * params_buffer;
	struct nand_size_info baseInfo;		/* nand information */
	char cmdline_buffer[CMDLINE_LEN];	/* Linux command line */

	struct display_module *display_info;	/* display module */
	struct madrid_display_module  mad_disp_info;	/* added 5apr11 for madridOptimize */
} global_var;


u32 calc_SDRAM_ADDRESS(void);	/* return SDRAM base address	    */

void	     init_global(void);	/* startup.S does not clear data    */
global_var * get_global(void);	/* get ptr to global data structure */

#endif /* GLOBAL_H */

