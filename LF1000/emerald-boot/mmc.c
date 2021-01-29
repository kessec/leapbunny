/* mmc.c - Basic polling MMC/SD driver
 *
 * Copyright 2010-2011 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * This code is partly based on the MMC/SDIO stack implementation in the Linux
 * kernel which is Copyright 2006-2007 Pierre Ossman.
 *
 * FIXME: read from FIFO
 *
 * TODO: switch to 4-bit mode only if card CSR says it's supported, right now
 *       we assume that 4-bit mode is supported.
 * TODO: support different sector sizes (querry SD card)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdint.h>
#include <common.h>
#include <base.h>
#include <gpio.h>
#include <board.h>
#include <sdhc.h>
#include <debug.h>

#ifdef MMC_DEBUG
#define mmc_err(...)	serio_puts("mmc: " __VA_ARGS__)
#else
#define mmc_err(...)
#endif

#define mmc_info(...)	serio_puts("mmc: " __VA_ARGS__)

/* Base address */
#define MMC(x)		REG32(sdhc_base+x)

/* Clock settings */

#define MMC_CLK_DIV	3	/* PLL1/3 gives 147MHz/3 = 49MHz */
#define MMC_DIV_400KHZ	62	/* divider for 400KHz: 49MHz/(62*2) */
#define MMC_DIV_FULL	0	/* divider for full speed */

#define MMC_RETRY_MAX	10000
#define MMC_RETRY_ERR	0xFFFF0000

#define MMC_BLOCK_LEN	512

/* MMC response types mapped to host controller flags */

#define MMC_RESP_R1		(1<<RSPEXP)|(1<<CHKRSPCRC)
#define MMC_RESP_R1B		(1<<RSPEXP)|(1<<CHKRSPCRC)
#define MMC_RESP_R2		(1<<RSPEXP)|(1<<RSPLEN)|(1<<CHKRSPCRC)
#define MMC_RESP_R3		(1<<RSPEXP)
#define MMC_RESP_R4		(1<<RSPEXP)
#define MMC_RESP_R5		(1<<RSPEXP)|(1<<CHKRSPCRC)
#define MMC_RESP_R6		(1<<RSPEXP)|(1<<CHKRSPCRC)
#define MMC_RESP_R7		(1<<RSPEXP)|(1<<CHKRSPCRC)

/* MMC command opcodes mapped to host controller flags */

#define MMC_COMMAND(op, fl)	(op | fl)

#define SD_CMD_GO_IDLE_STATE		MMC_COMMAND(0,  (1<<SENDINIT))
#define SD_CMD_SEND_OP_COND		MMC_COMMAND(1,  MMC_RESP_R3)
#define SD_CMD_ALL_SEND_CID		MMC_COMMAND(2,  MMC_RESP_R2)
#define SD_CMD_SEND_REL_ADDR		MMC_COMMAND(3,  MMC_RESP_R1)
#define SD_CMD_SELECT_CARD		MMC_COMMAND(7,	MMC_RESP_R1)
#define SD_CMD_SEND_IF_COND		MMC_COMMAND(8,  MMC_RESP_R1)
#define MMC_CMD_SEND_CSD		MMC_COMMAND(9,	MMC_RESP_R2)
#define SD_CMD_STOP_TRANSMISSION	MMC_COMMAND(12, MMC_RESP_R1)
#define SD_CMD_SEND_STATUS		MMC_COMMAND(13, MMC_RESP_R1)
#define SD_CMD_SET_BLOCKLEN		MMC_COMMAND(16, MMC_RESP_R1)
#define SD_CMD_READ_SINGLE_BLOCK	MMC_COMMAND(17, MMC_RESP_R1|(1<<DATEXP))
#define SD_CMD_READ_MULTIPLE_BLOCK	MMC_COMMAND(18, MMC_RESP_R1|(1<<DATEXP))
#define SD_CMD_APP_OP_COND		MMC_COMMAND(41, MMC_RESP_R3)
#define SD_CMD_APP_CLR_CD		MMC_COMMAND(42, MMC_RESP_R1)
#define SD_CMD_APP			MMC_COMMAND(55, MMC_RESP_R1)
#define SD_CMD_APP_SET_BUS_WIDTH	MMC_COMMAND(6,  MMC_RESP_R1)

#ifdef MMC_DEBUG
/* decode card info (from Linux kernel drivers/mmc/core/sd.c) */
static unsigned int unstuff_bits(u32 *resp, u32 start, u32 size)
{
	const int __size = size;                                
	const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1; 
	const int __off = 3 - ((start) / 32);                   
	const int __shft = (start) & 31;                        
	u32 __res;                                              

	__res = resp[__off] >> __shft;                          
	if (__size + __shft > 32)                               
		__res |= resp[__off-1] << ((32 - __shft) % 32); 
	return __res & __mask;                                         
}
#endif

static struct {
	u32 resp[4];
	u32 rca;

	u32 *data_buf;
	
	unsigned high_capacity : 1;
} mmc;

static inline void mmc_clear_int_status(u32 sdhc_base)
{
	MMC(SDI_RINTSTS) = 0xFFFFFFFF;
}

static void mmc_reset_fifo(u32 sdhc_base)
{
	u32 tmp;
	unsigned int i = 0;

	tmp = MMC(SDI_CTRL) & ~((1<<CTRLRST)|(1<<DMARST));
	MMC(SDI_CTRL) = (tmp | (1<<FIFORST));
	while ((MMC(SDI_CTRL) & (1<<FIFORST)) && ++i < MMC_RETRY_MAX);
}

static int mmc_wait_for_cmd(u32 sdhc_base)
{
	unsigned int i = 0;

	while ((MMC(SDI_CMD) & (1<<STARTCMD)) && ++i < MMC_RETRY_MAX);

	return i >= MMC_RETRY_MAX;
}

static int mmc_update_clock(u32 sdhc_base)
{
	unsigned int i = 0;

	do {
		MMC(SDI_RINTSTS) = (1<<HLEINT);
		MMC(SDI_CMD) = (1<<STARTCMD)|(1<<UPDATECLKONLY)|(1<<WAITPRVDAT);
		mmc_wait_for_cmd(sdhc_base);
	} while ((MMC(SDI_RINTSTS) & (1<<HLEINT)) && ++i < MMC_RETRY_MAX);

	return i >= MMC_RETRY_MAX;
}

static int mmc_set_clock(u32 sdhc_base, u8 div)
{
	MMC(SDI_CLKENA) &= ~(1<<CLKENA);
	MMC(SDI_CLKDIV) = div;

	mmc_update_clock(sdhc_base);

	MMC(SDI_CLKENA) |= (1<<CLKENA);

	mmc_update_clock(sdhc_base);

	MMC(SDI_SYSCLKENB) = (1<<PCLKMODE)|(1<<CLKGENENB);

	return 0;
}

#define STATUS_TXFER_BUSY	((1<<FSMBUSY)|(1<<FIFOEMPTY))

static int mmc_send_command(u32 sdhc_base, u32 cmd, u32 arg )
{
	u32 irqm = (1<<MSKHLE)|(1<<MSKRE)|(1<<MSKCD);
	unsigned int i;

	db_puts("mmc_send_command  sdhc_base:");db_int(sdhc_base);
	db_puts(" cmd:"); db_int(cmd);
	db_puts(" arg:"); db_int(arg); db_puts("\n");

	cmd |= (1<<STARTCMD);
	if ((cmd & 0x3F) != 13)
		cmd |= (1<<WAITPRVDAT);

	if (cmd & (1<<RSPEXP))
		irqm |= (1<<MSKRTO);
	if (cmd & (1<<DATEXP)) /* data transfer (assume RX) */
		irqm |= (1<<MSKDTO)|(1<<MSKHTO)|(1<<MSKFRUN)|(1<<MSKDRTO);

	/* send the command */
	do {
		mmc_clear_int_status(sdhc_base);
		MMC(SDI_INTMASK) = irqm;
		MMC(SDI_CMDARG) = arg;
		MMC(SDI_CMD) = cmd;
		i = 0;
		while ((MMC(SDI_CMD) & 1<<STARTCMD) && ++i < MMC_RETRY_MAX);
		if (i >= MMC_RETRY_MAX) {
			db_puts("mmc_send_command.");
			db_int(__LINE__);db_puts(": return 1\n");
			return 1;
		}
	} while (MMC(SDI_MINTSTS) & (1<<MSKHLE));

	/* reset the FIFO if data transfer is busy */
	if ((MMC(SDI_STATUS) & STATUS_TXFER_BUSY) == STATUS_TXFER_BUSY)
		mmc_reset_fifo(sdhc_base);

	/* wait until the command response comes back*/
	i = 0;
	while (!(MMC(SDI_MINTSTS) & (1<<MSKCD)) && ++i < MMC_RETRY_MAX);
	if (i >= MMC_RETRY_MAX) {
		db_puts("mmc_send_command.");
		db_int(__LINE__);db_puts(": return 2\n");
		return 2;
	}
	
	MMC(SDI_RINTSTS) = (1<<MSKCD);

	/* check for command response errors */
	if (MMC(SDI_MINTSTS) & ((1<<REINT)|(1<<RCRCINT)|(1<<RTOINT))) {
#ifdef MMC_DEBUG
		if (MMC(SDI_RINTSTS) & (1<<REINT))
			mmc_err("error RE\n");
		if (MMC(SDI_RINTSTS) & (1<<RCRCINT))
			mmc_err("error RCRC\n");
		if (MMC(SDI_RINTSTS) & (1<<RTOINT))
			mmc_err("error RTO\n");
#endif /* MMC_DEBUG */
		db_puts("mmc_send_command.");
		db_int(__LINE__);db_puts(": return 3\n");
		return 3;
	}

	/* read the response */
	if (cmd & (1<<RSPEXP)) {
		if (cmd & (1<<RSPLEN)) { /* long response */
			mmc.resp[0] = MMC(SDI_RESP3);
			mmc.resp[1] = MMC(SDI_RESP2);
			mmc.resp[2] = MMC(SDI_RESP1);
			mmc.resp[3] = MMC(SDI_RESP0);
			db_puts(":");
			db_int(mmc.resp[0]);
			db_puts(":");
			db_int(mmc.resp[1]);
			db_puts(":");
			db_int(mmc.resp[2]);
			db_puts(":");
			db_int(mmc.resp[3]);
			db_puts("\n");
		} else
		{
			mmc.resp[0] = MMC(SDI_RESP0);
			db_puts(":");
			db_int(mmc.resp[0]);
			db_puts("\n");
		}
	}

	return 0;
}

static int mmc_send_app_command(u32 sdhc_base, u32 cmd, u32 arg)
{
	int i;
	int ret;

	for (i = 0; i < 100; i++) {
		ret = mmc_send_command(sdhc_base, SD_CMD_APP, mmc.rca);
		if (ret)
			return 1;

		ret = mmc_send_command(sdhc_base, cmd, arg);
		if (ret == 0)
			return 0;
	}

	return ret;
}

static int mmc_detect(u32 sdhc_base)
{
	u32 hcs = 0;
	unsigned int i;
	int ret;

	mmc.rca = 0;

	if (mmc_set_clock(sdhc_base, MMC_DIV_400KHZ)) {
		mmc_err("failed to set clock\n");
		return 1;
	}

	mmc_send_command(sdhc_base, SD_CMD_GO_IDLE_STATE, 0);
	
	/* check for VHS 2.7-3.6V, check pattern 0xAA */
	ret = mmc_send_command(sdhc_base, SD_CMD_SEND_IF_COND, (1<<8)|0xAA);
	if (ret)
		mmc_err("voltage mismatch?\n");
	else {
		if (mmc.resp[0] == ((1<<8)|0xAA)) {
			mmc_info("detected SD 2.0 or later");
			hcs = (1<<30);
		} else {
			mmc_info("no card detected\n");
			return 2;
		}
	}

	/* validate voltage (3.0-3.6V) and wait for powerup */
	i = 0;
	do {
		ret = mmc_send_app_command(sdhc_base, SD_CMD_APP_OP_COND,
				hcs | 0x00FC0000);
		if (ret)
			return 3;
	} while (!(mmc.resp[0] & (1<<31)) && ++i < MMC_RETRY_MAX);

	if (i >= MMC_RETRY_MAX || (mmc.resp[0] & 0x00FC0000) != 0x00FC0000)
		return MMC_RETRY_ERR | 4;

	if (mmc.resp[0] & (1<<30)) {
		mmc.high_capacity = 1;
		serio_puts(", high-capacity");
	}
	serio_putchar('\n');

	/* get the card CID */

	if (mmc_send_command(sdhc_base, SD_CMD_ALL_SEND_CID, 0)) {
		mmc_err("failed to get CID\n");
		return 5;
	}

#ifdef MMC_DEBUG
	serio_puts("mmc: CID=");
	serio_int(mmc.resp[0]);
	serio_int(mmc.resp[1]);
	serio_int(mmc.resp[2]);
	serio_int(mmc.resp[3]);
	serio_puts("\nProduct: ");
	serio_putchar(unstuff_bits(mmc.resp, 96, 8));
	serio_putchar(unstuff_bits(mmc.resp, 88, 8));
	serio_putchar(unstuff_bits(mmc.resp, 80, 8));
	serio_putchar(unstuff_bits(mmc.resp, 72, 8));
	serio_putchar(unstuff_bits(mmc.resp, 64, 8));
	serio_puts("\n");
#endif

	/* get the RCA */
	if (mmc_send_command(sdhc_base, SD_CMD_SEND_REL_ADDR, 0)) {
		mmc_err("failed to send relative addr\n");
		return 6;
	}
	mmc.rca = mmc.resp[0] & 0xFFFF0000;

	return 0;
}

static inline int mmc_set_block_length(u32 sdhc_base, unsigned int len)
{
	return mmc_send_command(sdhc_base, SD_CMD_SET_BLOCKLEN, len);
}

static int mmc_set_bus_width(u32 sdhc_base, u8 width)
{
	int ret;

	if (width != 1 && width != 4)
		return 1;

	if ((width == 4 && MMC(SDI_CTYPE) & (1<<WIDTH)) ||
	    (width == 1 && !(MMC(SDI_CTYPE) & (1<<WIDTH))))
		return 0;

	ret = mmc_send_app_command(sdhc_base, SD_CMD_APP_SET_BUS_WIDTH,
				width>>1);
	if (ret)
		return ret;

	if (width == 1)
		MMC(SDI_CTYPE) &= ~(1<<WIDTH);
	else
		MMC(SDI_CTYPE) |= (1<<WIDTH);

	return 0;
}

static int mmc_select_card(u32 sdhc_base)
{
	return mmc_send_command(sdhc_base, SD_CMD_SELECT_CARD, mmc.rca);
}

static void mmc_pin_init(u32 sdhc_base)
{
	db_puts("mmc_pin_init: "); db_int(sdhc_base); db_puts("\n");

	switch (sdhc_base) {
	case SDHC0_BASE:
		gpio_configure_pin(GPIO_PORT_B,  GPIO_PIN0, GPIO_ALT1, 1, 0, 1);
		gpio_configure_pin(GPIO_PORT_B,  GPIO_PIN1, GPIO_ALT1, 1, 0, 1);
		gpio_configure_pin(GPIO_PORT_B,  GPIO_PIN2, GPIO_ALT1, 1, 0, 1);
		gpio_configure_pin(GPIO_PORT_B,  GPIO_PIN3, GPIO_ALT1, 1, 0, 1);
		gpio_configure_pin(GPIO_PORT_B,  GPIO_PIN4, GPIO_ALT1, 1, 0, 1);
		gpio_configure_pin(GPIO_PORT_B,  GPIO_PIN5, GPIO_ALT1, 1, 0, 1);

		gpio_set_cur(GPIO_PORT_B,  GPIO_PIN0, GPIO_CURRENT_2MA);
		gpio_set_cur(GPIO_PORT_B,  GPIO_PIN1, GPIO_CURRENT_2MA);
		gpio_set_cur(GPIO_PORT_B,  GPIO_PIN2, GPIO_CURRENT_2MA);
		gpio_set_cur(GPIO_PORT_B,  GPIO_PIN3, GPIO_CURRENT_2MA);
		gpio_set_cur(GPIO_PORT_B,  GPIO_PIN4, GPIO_CURRENT_2MA);
		gpio_set_cur(GPIO_PORT_B,  GPIO_PIN5, GPIO_CURRENT_2MA);
		break;

	case SDHC1_BASE:
		gpio_configure_pin(GPIO_PORT_B,  GPIO_PIN6, GPIO_ALT1, 1, 0, 1);
		gpio_configure_pin(GPIO_PORT_B,  GPIO_PIN7, GPIO_ALT1, 1, 0, 1);
		gpio_configure_pin(GPIO_PORT_B,  GPIO_PIN8, GPIO_ALT1, 1, 0, 1);
		gpio_configure_pin(GPIO_PORT_B,  GPIO_PIN9, GPIO_ALT1, 1, 0, 1);
		gpio_configure_pin(GPIO_PORT_B, GPIO_PIN10, GPIO_ALT1, 1, 0, 1);
		gpio_configure_pin(GPIO_PORT_B, GPIO_PIN11, GPIO_ALT1, 1, 0, 1);

		gpio_set_cur(GPIO_PORT_B,  GPIO_PIN6, GPIO_CURRENT_2MA);
		gpio_set_cur(GPIO_PORT_B,  GPIO_PIN7, GPIO_CURRENT_2MA);
		gpio_set_cur(GPIO_PORT_B,  GPIO_PIN8, GPIO_CURRENT_2MA);
		gpio_set_cur(GPIO_PORT_B,  GPIO_PIN9, GPIO_CURRENT_2MA);
		gpio_set_cur(GPIO_PORT_B, GPIO_PIN10, GPIO_CURRENT_2MA);
		gpio_set_cur(GPIO_PORT_B, GPIO_PIN11, GPIO_CURRENT_2MA);
		break;
	}
}

static void mmc_read8(u32 sdhc_base, u32 *dest)
{
	*dest++ = MMC(SDI_DAT);
	*dest++ = MMC(SDI_DAT);
	*dest++ = MMC(SDI_DAT);
	*dest++ = MMC(SDI_DAT);
	*dest++ = MMC(SDI_DAT);
	*dest++ = MMC(SDI_DAT);
	*dest++ = MMC(SDI_DAT);
	*dest++ = MMC(SDI_DAT);
}

/* Read the remaining amount of data left in the FIFO based on the FIFOCOUNT
 * found in the STATUS register. */
static void mmc_read(u32 sdhc_base, u32 *dest)
{
	int count = MMC(SDI_STATUS)>>FIFOCOUNT & 0x1F;
	
	for (; count > 0; count--)
		*dest++ = MMC(SDI_DAT);
}

int mmc_read_sector(u32 sdhc_base, u32 sector, u32 *dest)
{
	int ret;
	unsigned int i = 0, j = 0;

	db_puts("mmc_read_sector\n");

	if (!mmc.rca || !dest)
		return 1;

	/* wait until the card is ready for data (bit 8 in status) */
	do {
		ret = mmc_send_command(sdhc_base, SD_CMD_SEND_STATUS,
			mmc.rca);
		if (ret)
			return (ret<<16)|2;
	} while (!(mmc.resp[0] & (1<<8)) && ++i < MMC_RETRY_MAX);

	if (i >= MMC_RETRY_MAX)
		return MMC_RETRY_ERR | 2;

	MMC(SDI_BYTCNT) = 1*MMC_BLOCK_LEN;

	mmc.data_buf = dest;
	ret = mmc_send_command(sdhc_base, SD_CMD_READ_SINGLE_BLOCK,
			mmc.high_capacity ? sector: sector*MMC_BLOCK_LEN);
	if (ret)
		return (ret<<16)|3;

	i = 0;
	while (i < (1*MMC_BLOCK_LEN)/(8*sizeof(u32)) &&
			++j < 10*MMC_RETRY_MAX) {
		/* There's an 8-word chunk of data in the FIFO */
		if (MMC(SDI_STATUS) & (1<<RXWMARK)) {
			mmc_read8(sdhc_base, dest);
		       	dest += 8;
			i++;
			j = 0;
		}
	
		/* Transfer is over, read remaining data (if any) */
		if (MMC(SDI_MINTSTS) & (1<<DTOINT)) {
			mmc_read(sdhc_base, dest);
			MMC(SDI_RINTSTS) = (1<<DTOINT);
			mmc_reset_fifo(sdhc_base);
			j = 0;
			break;
		}
	}

	if (j >= MMC_RETRY_MAX) {
		mmc_err("Timeout on read\n");
		return MMC_RETRY_ERR | 4;
	}

	return 0;
}

int mmc_read_multiblock(u32 sdhc_base,  u32 sector, u32 num_sectors, u32 *dest)
{
	int ret;
	unsigned int i = 0;

	db_puts("mmc_read_multiblock\n");

	if (!mmc.rca || !dest)
		return 1;

	/* wait until the card is ready for data (bit 8 in status) */
	do {
		ret = mmc_send_command(sdhc_base, SD_CMD_SEND_STATUS,
			mmc.rca);
		if (ret)
			return (ret<<16)|2;
	} while (!(mmc.resp[0] & (1<<8)) && ++i < MMC_RETRY_MAX);

	if (i >= MMC_RETRY_MAX)
		return MMC_RETRY_ERR | 2;

	/* Fill in the number of bytes we'll be reading */
	MMC(SDI_BYTCNT) = num_sectors * MMC_BLOCK_LEN;

	mmc.data_buf = dest;
	ret = mmc_send_command(sdhc_base, SD_CMD_READ_MULTIPLE_BLOCK,
			mmc.high_capacity ? sector: sector*MMC_BLOCK_LEN);
	if (ret)
		return (ret<<16)|3;

	i = 0;
	while ( i < ( num_sectors * MMC_BLOCK_LEN ) / ( 8 * sizeof(u32) ) ) {
		/* There's an 8-word chunk of data in the FIFO */
		if (MMC(SDI_STATUS) & (1<<RXWMARK)) {
			mmc_read8(sdhc_base, dest);
		       	dest += 8;
			i++;
		}
	}
	
	/* Transfer is over, Send Stop */
	mmc_send_command(sdhc_base, SD_CMD_STOP_TRANSMISSION, 0);
	mmc_reset_fifo(sdhc_base);

	return 0;
	
}


int mmc_init(u32 sdhc_base)
{
	int i, ret;
	u32 tmp;

	db_puts("mmc_init."); db_int(__LINE__); db_puts(": ");
	db_int(sdhc_base); db_puts("\n");

	for (i = 0; i < sizeof(mmc); i++)
		*(((char *)&mmc)+i) = 0;

	mmc_pin_init(sdhc_base);

	MMC(SDI_CLKENA) = 0;
	MMC(SDI_CLKGEN) = (2<<CLKSRCSEL0)|((MMC_CLK_DIV-1)<<CLKDIV0);
	MMC(SDI_SYSCLKENB) = (1<<PCLKMODE)|(1<<CLKGENENB);
	MMC(SDI_CLKDIV) = MMC_DIV_400KHZ;

	MMC(SDI_CTRL) |= (1<<INT_ENA);

	/* reset the controller */
	tmp = MMC(SDI_CTRL) & ~((1<<DMARST)|(1<<FIFORST));
	MMC(SDI_CTRL) = (tmp | (1<<CTRLRST));
	
	db_puts("mmc_init."); db_int(__LINE__); db_puts("\n");

	i = 0;
	while ((MMC(SDI_CTRL) & (1<<CTRLRST)) && ++i < MMC_RETRY_MAX);
	if (i >= MMC_RETRY_MAX) {
		db_puts("mmc_init."); db_int(__LINE__); db_puts(" return 1\n");
		return 1;
	}

	db_puts("mmc_init."); db_int(__LINE__); db_puts("\n");

	/* reset the DMA */
	tmp = MMC(SDI_CTRL) & ~((1<<CTRLRST)|(1<<FIFORST));
	MMC(SDI_CTRL) = (tmp | (1<<DMARST));
	i = 0;
	while ((MMC(SDI_CTRL) & (1<<DMARST)) && ++i < MMC_RETRY_MAX);
	if (i >= MMC_RETRY_MAX) {
		db_puts("mmc_init."); db_int(__LINE__); db_puts(" return 2\n");
		return 2;
	}

	mmc_reset_fifo(sdhc_base);

	MMC(SDI_CTRL) &= ~(1<<DMA_ENA); /* no DMA */
	MMC(SDI_CTYPE) &= ~(1<<WIDTH); /* 1-bit mode */
	MMC(SDI_TMOUT) = (0xFFFFFF<<DTMOUT)|(0xFF<<RSPTMOUT);

	MMC(SDI_BLKSIZ) = MMC_BLOCK_LEN;
	MMC(SDI_FIFOTH) = ((8-1)<<RXTH)|(8<<TXTH);
	MMC(SDI_INTMASK) = 0;
	mmc_clear_int_status(sdhc_base);

	/* now try to find a card and initialize it */
	ret = mmc_detect(sdhc_base);
	if (ret) {
		db_puts("mmc_init."); db_int(__LINE__); db_puts(" return 3\n");
		mmc_err("didn't find a card\n");
		return ret;
	}

	if (mmc_set_clock(sdhc_base, MMC_DIV_FULL))
		mmc_err("failed to set full clock speed\n");
	if (mmc_select_card(sdhc_base))
		mmc_err("failed to select card\n");
	if (mmc_send_app_command(sdhc_base, SD_CMD_APP_CLR_CD, 0))
		mmc_err("failed to disable card detect pullup\n");
	if (mmc_set_bus_width(sdhc_base, 4))
		mmc_err("failed to set 4-bit mode\n");
	if (mmc_set_block_length(sdhc_base, MMC_BLOCK_LEN))
		mmc_err("failed to set block length\n");
	
	return 0;
}

void mmc_exit(u32 sdhc_base)
{
//	mmc_send_command(sdhc_base, SD_CMD_GO_IDLE_STATE, 0);
	mmc_clear_int_status(sdhc_base);

	MMC(SDI_CTRL) &= ~(1<<DMA_ENA); /* no DMA */
	MMC(SDI_CTRL) &= ~(1<<INT_ENA); /* no INT */
	MMC(SDI_SYSCLKENB) &= ~(1<<CLKGENENB); /* no CLK */
}
