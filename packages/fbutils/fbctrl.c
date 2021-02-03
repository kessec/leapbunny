/* fbctrl.c -- additional controls for the LF1000 frame buffer.  This utility
 *             excersizes some LF1000-specific extensions and provides easy
 *             access to a few standard features as well.
 *
 * Copyright (c) 2010 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <string.h>
#include <linux/fb.h>
#include <linux/lf1000/lf1000fb.h>

#define u8	uint8_t
#define u32	uint32_t

#include "check-fb.h"

struct fbctrl_cmd {
	const char *name;
	const char *help;
	int(*get)(int fd);
	int(*set)(int fd, char **argv, int offs);
	unsigned int nargs;
};

int get_blend(int fd)
{
	struct lf1000fb_blend_cmd cmd;
	int ret;
	
	ret = ioctl(fd, LF1000FB_IOCGALPHA, &cmd);
	if (ret)
		perror("ioctl get alpha blend");
	else
		printf("%sabled (alpha: %u)\n",
				cmd.enable ? "en" : "dis", cmd.alpha);

	return ret;
}

int set_blend(int fd, char **argv, int offs)
{
	struct lf1000fb_blend_cmd cmd;
	int ret;

	if (sscanf(argv[offs], "%hhu", &cmd.enable) != 1)
		return 1;
	if (sscanf(argv[offs+1], "%hhu", &cmd.alpha) != 1)
		return 1;

	ret = ioctl(fd, LF1000FB_IOCSALPHA, &cmd);
	if (ret)
		perror("ioctl set alpha blend");

	return ret;
}

int get_scaler(int fd)
{
	struct lf1000fb_vidscale_cmd cmd;
	int ret;

	ret = ioctl(fd, LF1000FB_IOCGVIDSCALE, &cmd);
	if (ret) {
		perror("ioctl get scaler");
		return ret;
	}

	printf("scale from: %ux%u\n", cmd.sizex, cmd.sizey);
	return 0;
}

int set_scaler(int fd, char **argv, int offs)
{
	struct lf1000fb_vidscale_cmd cmd;
	int ret;
	unsigned int apply;

	if (sscanf(argv[offs], "%u", &cmd.sizex) != 1)
		return 1;
	if (sscanf(argv[offs+1], "%u", &cmd.sizey) != 1)
		return 1;
	if (sscanf(argv[offs+2], "%u", &apply) != 1)
		return 1;

	cmd.apply = apply;

	ret = ioctl(fd, LF1000FB_IOCSVIDSCALE, &cmd);
	if (ret)
		perror("ioctl set scaler");

	return ret;
}

int get_position(int fd)
{
	struct lf1000fb_position_cmd cmd;
	int ret;

	ret = ioctl(fd, LF1000FB_IOCGPOSTION, &cmd);
	if (ret) {
		perror("ioctl get position");
		return ret;
	}

	printf("left: %d, top: %d, right: %d, bottom: %d\n", cmd.left, cmd.top, cmd.right, cmd.bottom);
	return 0;
}

int set_position(int fd, char **argv, int offs)
{
	struct lf1000fb_position_cmd cmd;
	int ret;
	unsigned int apply;

	if (sscanf(argv[offs], "%d", &cmd.left) != 1)
		return 1;
	if (sscanf(argv[offs+1], "%d", &cmd.top) != 1)
		return 1;
	if (sscanf(argv[offs+2], "%d", &cmd.right) != 1)
		return 1;
	if (sscanf(argv[offs+3], "%d", &cmd.bottom) != 1)
		return 1;
	if (sscanf(argv[offs+4], "%u", &apply) != 1)
		return 1;

	cmd.apply = apply;

	ret = ioctl(fd, LF1000FB_IOCSPOSTION, &cmd);
	if (ret)
		perror("ioctl set position");

	return ret;
}

int set_blank(int fd, char **argv, int offs)
{
	int val;
	int ret;

	if (sscanf(argv[offs], "%d", &val) != 1)
		return -EINVAL;
	
	ret = ioctl(fd, FBIOBLANK, val);
	if (ret)
		perror("ioctl set blank");
	
	return ret;
}

int get_priority(int fd)
{
	struct fb_var_screeninfo vinfo;
	int ret;
	
	ret = ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
	if (ret) {
		perror("ioctl get var info");
		return ret;
	}

	printf("%d\n", (((vinfo.nonstd) >> 24) & 0x3));
	return 0;
}

int set_priority(int fd, char **argv, int offs)
{
	struct fb_var_screeninfo vinfo;
	int ret;
	uint8_t priority;

	if (sscanf(argv[offs], "%hhu", &priority) != 1)
		return -EINVAL;

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
	if (ret) {
		perror("ioctl get var info");
		return ret;
	}

	vinfo.nonstd &= ~(3<<24);
	vinfo.nonstd |= ((priority & 0x3))<<24;

	ret = ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo);
	if (ret) {
		perror("ioctrl set var info");
		return ret;
	}

	return 0;
}

int set_flip(int fd, char **argv, int offs)
{
	struct fb_var_screeninfo vinfo;
	int ret;
	unsigned int flip;

	if (sscanf(argv[offs], "%u", &flip) != 1)
		return -EINVAL;

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
	if (ret) {
		perror("ioctl get var info");
		return ret;
	}

	vinfo.rotate = flip ? 180 : 0;

	ret = ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo);
	if (ret) {
		perror("ioctrl set var info");
		return ret;
	}

	return 0;
}

int get_pan(int fd)
{
	struct fb_var_screeninfo vinfo;
	int ret;

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
	if (ret) {
		perror("ioctl get fixed info");
		return ret;
	}

	printf("xoffset: %u, yoffset: %u\n", vinfo.xoffset, vinfo.yoffset);
	return 0;
}

int set_pan(int fd, char **argv, int offs)
{
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	int ret;
	unsigned int pan[2];

	if (sscanf(argv[offs], "%u", &pan[0]) != 1)
		return -EINVAL;
	if (sscanf(argv[offs+1], "%u", &pan[1]) != 1)
		return -EINVAL;

	ret = ioctl(fd, FBIOGET_FSCREENINFO, &finfo);
	if (ret) {
		perror("ioctl get fixed info");
		return ret;
	}

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
	if (ret) {
		perror("ioctl get var info");
		return ret;
	}

	if (finfo.xpanstep && !(pan[0] % finfo.xpanstep))
		vinfo.xoffset = pan[0];
	if (finfo.ypanstep && !(pan[1] % finfo.ypanstep))
		vinfo.yoffset = pan[1];

	ret = ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo);
	if (ret) {
		perror("ioctrl set var info");
		return ret;
	}

	return 0;
}

int get_resolution(int fd)
{
	struct fb_var_screeninfo vinfo;
	int ret;

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
	if (ret) {
		perror("ioctl get var info");
		return ret;
	}

	printf("%ux%u (virtual: %ux%u)\n", vinfo.xres, vinfo.yres,
			vinfo.xres_virtual, vinfo.yres_virtual);
	return 0;
}

int set_resolution(int fd, char **argv, int offs)
{
	struct fb_var_screeninfo vinfo;
	int ret;

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
	if (ret) {
		perror("ioctl get var info");
		return ret;
	}

	if (sscanf(argv[offs], "%u", &vinfo.xres) != 1)
		return -EINVAL;
	if (sscanf(argv[offs+1], "%u", &vinfo.yres) != 1)
		return -EINVAL;

	ret = ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo);
	if (ret) {
		perror("ioctrl set var info");
		return ret;
	}

	return 0;
}

int get_format(int fd)
{
	struct fb_var_screeninfo vinfo;
	int ret;
	
	ret = ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
	if (ret) {
		perror("ioctl get var info");
		return ret;
	}

	switch (((vinfo.nonstd) >> 20) & 0x7) {
		case 0:
			printf("RGB %d\n", vinfo.bits_per_pixel);
			printf("red\t%d @ %d\n", vinfo.red.length,
					vinfo.red.offset);
			printf("green\t%d @ %d\n", vinfo.green.length,
					vinfo.green.offset);
			printf("blue\t%d @ %d\n", vinfo.blue.length,
					vinfo.blue.offset);
			printf("transp\t%d @ %d\n", vinfo.transp.length,
					vinfo.transp.offset);
			break;
		case 1:
			printf("YUV420\n");
			break;
		case 2:
			printf("YUV422\n");
			break;
		default:
			printf("unknown\n");
			break;
	}

	return 0;
}

int set_order(int fd, char **argv, int offs)
{
	struct fb_var_screeninfo vinfo;
	int ret;

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
	if (ret) {
		perror("ioctl get var info");
		return ret;
	}

	if (!strcmp(argv[offs], "RGB")) {
		switch (vinfo.bits_per_pixel) {
			case 16:
				vinfo.blue.offset = 0;
				vinfo.green.offset = 5;
				vinfo.red.offset = 11;
				break;
			case 24:
			case 32:
				vinfo.blue.offset = 0;
				vinfo.green.offset = 8;
				vinfo.red.offset = 16;
				break;
		}
	} else if (!strcmp(argv[offs], "BGR")) {
		switch (vinfo.bits_per_pixel) {
			case 16:
				vinfo.blue.offset = 11;
				vinfo.green.offset = 5;
				vinfo.red.offset = 0;
				break;
			case 24:
			case 32:
				vinfo.blue.offset = 16;
				vinfo.green.offset = 8;
				vinfo.red.offset = 0;
				break;
		}
	} else
		return -EINVAL;

	ret = ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo);
	if (ret) {
		perror("ioctrl set var info");
		return ret;
	}

	return 0;
}

int get_order(int fd)
{
	struct fb_var_screeninfo vinfo;
	int ret;

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
	if (ret) {
		perror("ioctl get var info");
		return ret;
	}

	if (((vinfo.nonstd) >> 20) & 0x7) {
		fprintf(stderr, "YUV layer\n");
		return -EINVAL;
	}

	if (vinfo.blue.offset < vinfo.green.offset &&
			vinfo.green.offset < vinfo.red.offset)
		printf("RGB\n");
	else if (vinfo.red.offset < vinfo.green.offset &&
		       vinfo.green.offset < vinfo.blue.offset)
		printf("BGR\n");
	else
		printf("unknown\n");

	return 0;
}

int get_type(int fd)
{
	struct fb_fix_screeninfo finfo;
	int ret;

	ret = ioctl(fd, FBIOGET_FSCREENINFO, &finfo);
	if (ret) {
		perror("ioctl get fix info");
		return ret;
	}

	switch (finfo.type) {
		case FB_TYPE_PLANES:
			printf("planes\n");
			break;
		case FB_TYPE_PACKED_PIXELS:
			printf("pixels\n");
			break;
		default:
			printf("unknown\n");
			break;
	}

	return 0;
}

int set_type(int fd, char **argv, int offs)
{
	struct fb_var_screeninfo vinfo;
	int ret;

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
	if (ret) {
		perror("ioctl get var info");
		return ret;
	}

	if (!strcmp(argv[offs], "pixels"))
		vinfo.nonstd &= ~(1<<LF1000_NONSTD_PLANAR);
	else if (!strcmp(argv[offs], "planes"))
		vinfo.nonstd |= (1<<LF1000_NONSTD_PLANAR);
	else
		return -EINVAL;

	ret = ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo);
	if (ret) {
		perror("ioctrl set var info");
		return ret;
	}

	return 0;
}

int get_address(int fd)
{
	struct fb_fix_screeninfo finfo;
	int ret;

	ret = ioctl(fd, FBIOGET_FSCREENINFO, &finfo);
	if (ret) {
		perror("ioctl get fix info");
		return ret;
	}

	printf("0x%lX (size: 0x%X)\n", finfo.smem_start, finfo.smem_len);
	return 0;
}

int get_offset(int fd)
{
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;
	int ret;

	ret = ioctl(fd, FBIOGET_FSCREENINFO, &finfo);
	if (ret) {
		perror("ioctl get fix info");
		return ret;
	}

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
	if (ret) {
		perror("ioctrl get var info");
		return ret;
	}

	printf("0x%08X (stride: 0x%X)\n", finfo.line_length * vinfo.yoffset + vinfo.xoffset, finfo.line_length);
	return 0;
}

int get_vblank(int fd)
{
	int ret;
	struct fb_vblank vblank;

	ret = ioctl(fd, FBIOGET_VBLANK, &vblank);
	if (ret) {
		perror("ioctl get vblank info");
		return ret;
	}
	printf("flags:     0x%08X\n", vblank.flags);
	printf("count:     %u\n", vblank.count);
	printf("vcount:    %u\n", vblank.vcount);
	printf("hcount:    %u\n", vblank.hcount);
	printf("\n");
	return 0;
}

struct fbctrl_cmd cmds[] = {
	{ "blend", "<en> <alpha>", get_blend, set_blend, 2 },
	{ "blank", "<0|1>", NULL, set_blank, 1 },
	{ "priority", "<val>", get_priority, set_priority, 1 },
	{ "format", NULL, get_format, NULL, 0 },
	{ "position", "<left> <top> <right> <bottom> <apply>", get_position, set_position, 5 },
	{ "scaler", "<sizex> <sizey> <apply>", get_scaler, set_scaler, 3 },
	{ "flip", "<0|1>", NULL, set_flip, 1 },
	{ "address", NULL, get_address, NULL, 0 },
	{ "offset", NULL, get_offset, NULL, 0 },
	{ "pan", "<xoffset> <yoffset>", get_pan, set_pan, 2 },
	{ "order", "RGB|BGR", get_order, set_order, 1 },
	{ "type", "pixels|planes", get_type, set_type, 1 },
	{ "vblank", NULL, get_vblank, NULL, 0 }
};

void print_usage(char *name)
{
	int i;

	printf("usage: %s <device> <get|set> <command> [args]\n", name);

	for (i = 0; i < sizeof(cmds)/sizeof(cmds[0]); i++) {
		if (cmds[i].get)
			printf("\t%s <device> get %s\n", name, cmds[i].name);
		if (cmds[i].set)
			printf("\t%s <device> set %s %s\n", name, cmds[i].name,
					cmds[i].help);
	}
}

int main(int argc, char **argv)
{
	int fbdev;
	int i;
	int ret = 0;

	if (!have_framebuffer()) {
		printf("Error: this tool must be used with FB graphics\n");
		return 1;
	}

	if (argc < 4) {
		print_usage(argv[0]);
		return 0;
	}

	fbdev = open(argv[1], O_RDWR|O_SYNC);
	if (fbdev < 0) {
		perror("open FB device");
		return 1;
	}

	if (!strcmp(argv[2], "get")) {
		for (i = 0; i < sizeof(cmds)/sizeof(cmds[0]); i++) {
			if (cmds[i].get && !strcmp(cmds[i].name, argv[3])) {
				ret = cmds[i].get(fbdev);
				break;
			}
		}

		if (i >= sizeof(cmds)/sizeof(cmds[0])) {
			fprintf(stderr, "invalid argument\n");
			print_usage(argv[0]);
		}
	} else if (!strcmp(argv[2], "set")) {
		for (i = 0; i < sizeof(cmds)/sizeof(cmds[0]); i++) {
			if (cmds[i].set && argc - 4 >= cmds[i].nargs &&
					!strcmp(cmds[i].name, argv[3])) {
				ret = cmds[i].set(fbdev, argv, 4);
				break;
			}
		}

		if (i >= sizeof(cmds)/sizeof(cmds[0])) {
			fprintf(stderr, "invalid argument\n");
			print_usage(argv[0]);
		}
	} else
		print_usage(argv[0]);

	if (ret) {
		fprintf(stderr, "error: %d\n", ret);
		print_usage(argv[0]);
	}

	close(fbdev);
	return ret;
}
