/*
 * This file contains some magic numbers supplied by PixArt.
 *
 * It is responsible for installing the Extention Unit (xu) controls (a.k.a.
 * dynamic controls), and using them to actually iterrogate/update the EEPROM.
 */
#include "common.h"
#include "pixart_ctrls.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>

#include <uvcvideo.h>

#include <stdio.h>
#include <errno.h>

#define MAX_STR_LEN	256
#define XU_UNIT_ID	4

#define XU_UNDEFINED	0
#define XU_BANK_2		3
#define XU_SET_ADDRESS	5
#define XU_EEROM_ER		6
#define XU_EEROM_WR		7

#define	IC_ATMEL		0x0001
#define	IC_MXIC			0x0002

/* PixArt PAC7332 Extension Unit GUID: {f07735d1-898d-0047-812e-7dd5e2fdb898} */

#define UVC_GUID_PIXART_XU {0xf0, 0x77, 0x35, 0xd1, 0x89, 0x8d, 0x00, 0x47, 0x81, 0x2e, 0x7d, 0xd5, 0xe2, 0xfd, 0xb8, 0x98}

int install_xu_ctrls(int fd)
{
	int i = 0, ret;

	struct uvc_xu_control_info xu[] =
	{
		/* CS = 3 : seek to bank2 */
		{
			.entity		= UVC_GUID_PIXART_XU,
			.index		= 2,
			.selector	= XU_BANK_2,
			.size		= 4,
			.flags		= UVC_CONTROL_GET_CUR | UVC_CONTROL_SET_CUR,
		},

		/* CS = 5 : seek to register offset */
		{
			.entity		= UVC_GUID_PIXART_XU,
			.index		= 4,
			.selector	= XU_SET_ADDRESS,
			.size		= 4,
			.flags		= UVC_CONTROL_SET_CUR,
		},

		/* CS = 6 : erase eeprom */
		{
			.entity		= UVC_GUID_PIXART_XU,
			.index		= 5,
			.selector	= XU_EEROM_ER,
			.size		= 4,
			.flags		= UVC_CONTROL_GET_CUR | UVC_CONTROL_SET_CUR,
		},

		/* CS = 7 : program eeprom */
		{
			.entity		= UVC_GUID_PIXART_XU,
			.index		= 6,
			.selector	= XU_EEROM_WR,
			.size		= 10,
			.flags		= UVC_CONTROL_GET_CUR | UVC_CONTROL_SET_CUR,
		},
	};

	for(i = 0; i < NUM_ELEMS(xu); i++)
	{
		xu[i]. index = i;
		if((ret = ioctl(fd, UVCIOC_CTRL_ADD, &xu[i])) != 0)
		{
			if(errno != EEXIST)
			{
				fprintf(stderr, "Failed to add control %d (%s)\n", xu[i].index, strerror(errno));
				break;
			}
			ret = 0;
		}
	}

	return ret;
}

static int read_reg(int dev, int bank, int addr, int *val)
{
	int ret;
	unsigned char data[4] = {0};

	struct uvc_xu_control seek =
	{
		.unit		= XU_UNIT_ID,
		.selector	= XU_SET_ADDRESS,
		.size		= 4,
		.data		= data,
	};

	struct uvc_xu_control read =
	{
		.unit		= XU_UNIT_ID,
		.selector	= bank,
		.size		= 4,
		.data		= data,
	};

	data[0] = addr;
	data[1] = 4;

	ret = ioctl(dev, UVCIOC_CTRL_SET, &seek);
	if(ret != 0) {printf("Error! %s\n", strerror(errno)); goto out;}

	ret = ioctl(dev, UVCIOC_CTRL_GET, &read);
	if(ret != 0) {printf("Error! %s\n", strerror(errno)); goto out;}

	*val = data[0];
	ret = 0;

	printf("%s: bank%d, addr=0x%.2X(%d), val= 0x%.2X(%d)\n",
		__FUNCTION__, bank, addr, addr, *val, *val);

out:
	return ret;
}

static int write_reg(int dev, int bank, int addr, int val)
{
    int ret;
    unsigned char data[4] = {0};

    struct uvc_xu_control write =
    {
        .unit       = XU_UNIT_ID,
        .selector   = bank,
        .size       = 4,
        .data       = data,
    };

    data[0] = addr;
    data[1] = val & 0xFF;
    data[2] = 1;
    data[3] = 0;

    ret = ioctl(dev, UVCIOC_CTRL_SET, &write);
    if(ret != 0) {printf("Error! %s\n", strerror(errno)); goto out;}

    ret = 0;

    printf("%s: bank%d, addr=0x%.2X(%d), val= 0x%.2X(%d)\n",
        __FUNCTION__, bank, addr, addr, val, val);

out:
    return ret;
}


FW_STATUS check_cam_fw(int dev, const struct fw_chunk *id_img)
{
	FW_STATUS status = FW_CHECK_ERR;

	int i, reg, ret;

    struct pix_reg_init {
        int bank;
        int addr;
        int data;
    };

	struct pix_reg {
		char str[MAX_STR_LEN];
		int reg;
		int bank;
		int off_high;
		int off_low;
	};

    struct pix_reg_init init[] =
    {
        {
            .bank       = 3,
            .addr       = 205,
            .data       = 0,
        },

        {
            .bank       = 3,
            .addr       = 214,
            .data       = 0,
        },

        {
            .bank       = 3,
            .addr       = 215,
            .data       = 0,
        },

        {
            .bank       = 3,
            .addr       = 217,
            .data       = 3,
        },

        {
            .bank       = 3,
            .addr       = 216,
            .data       = 16,
        },
    };

	struct pix_reg regs[] =
	{
		/* pid */
		{
			.str		= "PID",
			.reg		= 0,
			.bank		= 3,
			.off_high	= 190,
			.off_low	= 191,
		},
		/* vid */
		{
			.str		= "VID",
			.reg		= 0,
			.bank		= 3,
			.off_high	= 192,
			.off_low	= 193,
		},
		/* FW ckecksum */
		{
			.str		= "FW checksum",
			.reg		= 0,
			.bank		= 3,
			.off_high	= 194,
			.off_low	= 195,
		},
		/* HW ckecksum */
		{
			.str		= "HW checksum",
			.reg		= 0,
			.bank		= 3,
			.off_high	= 196,
			.off_low	= 197,
		},
		/* FW version */
		{
			.str		= "FW version",
			.reg		= 0,
			.bank		= 3,
			.off_high	= 198,
			.off_low	= 199,
		},
	};

	struct pix_reg img_reg[] =
	{
		/* pid */
		{
			.str		= "PID",
		},
		/* vid */
		{
			.str		= "VID",
		},
		/* FW ckecksum */
		{
			.str		= "FW checksum",
		},
		/* HW ckecksum */
		{
			.str		= "HW checksum (Dummy)",
		},
		/* FW version */
		{
			.str		= "FW version",
		},
	};

	/* TODO: pid and vid are switched ??? */
	struct pix_reg	*pid = &img_reg[1],
			*vid = &img_reg[0],
			*fcs = &img_reg[2],
			*hcs = &img_reg[3],
			*fwv = &img_reg[4];

#if 1
    /* init registers */
    for(i = 0; i < NUM_ELEMS(init); i++)
    {
        ret = write_reg(dev, init[i].bank, init[i].addr, init[i].data);
        if(ret != 0) {printf("error! ret=%d\n", ret); goto out;}
    }
#endif

	for(i = 0; i < NUM_ELEMS(regs); i++)
	{
		ret = read_reg(dev, regs[i].bank, regs[i].off_high, &reg);
		if(ret != 0) {printf("error! ret=%d\n", ret); goto out;}
		regs[i].reg = reg<<8;

		ret = read_reg(dev, regs[i].bank, regs[i].off_low, &reg);
		if(ret != 0) {printf("error! ret=%d\n", ret); goto out;}
		regs[i].reg |= reg;

		printf("%s: 0x%.4X\n", regs[i].str, regs[i].reg);
	}

	/* init UI.hex fields */
	pid->reg = id_img->data[0x8] << 8 | id_img->data[0x9];
	vid->reg = id_img->data[0x6] << 8 | id_img->data[0x7];
	fcs->reg = id_img->data[0x2] << 8 | id_img->data[0x3];
	hcs->reg = regs[3].reg;		/* Dummy */
	fwv->reg = id_img->data[0x4] << 8 | id_img->data[0x5];

	/* checksum mismatch */
	if(regs[2].reg != regs[3].reg)
	{
		status = FW_CHECK_MISMATCH;
		goto out;
	}

	status = FW_CHECK_MATCH;

	/* compare camera and UI.hex */
	for(i = 0; i < NUM_ELEMS(img_reg); i++)
	{
		if(img_reg[i].reg != regs[i].reg)
		{
			status =  FW_CHECK_MISMATCH;
			break;
		}
	}

out:
	return status;
}

int update_fw(int dev, const struct fw_chunk *img)
{
	int ret;
	unsigned char data[10] = {0};
	size_t offset = 0;
	static int erased = 0;

	struct uvc_xu_control erase =
	{
		.unit		= XU_UNIT_ID,
		.selector	= XU_EEROM_ER,
		.size		= 4,
		.data		= data,
	};

	struct uvc_xu_control program =
	{
		.unit		= XU_UNIT_ID,
		.selector	= XU_EEROM_WR,
		.size		= 10,
		.data		= data,
	};

	//data[0] = IC_ATMEL;
	data[0] = IC_MXIC;

	if(!erased)
	{
		fprintf(stdout, "Erasing EEPROM...");
		fflush(stdout);
		/* erase EEPROM */
		ret = ioctl(dev, UVCIOC_CTRL_SET, &erase);
		if(ret != 0)
			goto out;

		fprintf(stdout, "OK!\nSleeping...");
		fflush(stdout);
		/* 2 second sleep required */
		sleep(2);
		fprintf(stdout, "OK!\n");	
		erased = 1;
	}

	fprintf(stdout, "Programming offset %#x\n", img->offset);

	/* program in 8-byte chunks */
	while(offset <= (img->len - 8)) {
		memcpy(data, &img->data[offset], 8);
		data[8] = (img->offset + offset) & 0xFF;
		data[9] = ((img->offset + offset) >> 8);
		ret = ioctl(dev, UVCIOC_CTRL_SET, &program);
		if(ret != 0)
			goto out;

		offset += 8;
	}

	/* program remainder */
	if(img->len % 8)
	{
		bzero(data, 8);
		memcpy(data, &img->data[offset], img->len % 8);
		data[8] = (img->offset + offset) & 0xFF;
		data[9] = ((img->offset + offset) >> 8);
		ret = ioctl(dev, UVCIOC_CTRL_SET, &program);
	}

out:
	return ret;
}
