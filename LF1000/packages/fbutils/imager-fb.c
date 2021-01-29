/* imager-fb -- draw png images on a frame buffer
 *
 * Larger images will be clipped while smaller images will be centered and
 * framed with a black border.
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
#include <png.h>

#include "check-fb.h"
#include <linux/lf1000/lf1000fb.h>

/* draw a data buffer on the screen, center it (with a black background) if the
 * screen is larger than the image */
void draw(char *fb, unsigned char *data, struct fb_var_screeninfo *vinfo,
		unsigned long width, unsigned long height, unsigned long bpr)
{
	unsigned long xoff, yoff;
	unsigned int bpp = vinfo->bits_per_pixel>>3;
	unsigned int fbrow = vinfo->xres*bpp;
	int r;

	if (vinfo->xres != width || vinfo->yres != height)
		memset(fb, 0, bpp*vinfo->xres*vinfo->yres);

	if (vinfo->xres >= width) {
		xoff = (vinfo->xres - width)/2;
		if (vinfo->yres >= height)
			yoff = (vinfo->yres - height)/2;
		else {
			yoff = 0;
			height = vinfo->yres;
		}

		fb += yoff*vinfo->xres*bpp; /* skip vertical rows */

		for (r = 0; r < height; r++) {
			memcpy(fb + xoff*bpp, data, bpr);
			fb += fbrow;
			data += bpr;
		}
	} else {
		unsigned long aw = width < vinfo->xres ? width : vinfo->xres;
		unsigned long ah = height < vinfo->yres ? height : vinfo->yres;
		
		for (r = 0; r < ah; r++) {
			memcpy(fb, data, aw*bpp);
			fb += fbrow;
			data += bpr;
		}
	}
}

unsigned char *load_png(char *path, unsigned long *width,
		unsigned long *height, unsigned long *bpr, unsigned int *bpp,
		char *is_rgb)
{
	FILE *f;
	int bit_depth, color_type, interlace_type;
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int i, rowbytes;
	png_bytep *row_pointers;
	unsigned char *image_data;

	f = fopen(path, "rb");
	if (!f) {
		perror("open image file");
		return NULL;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
			NULL, NULL, NULL);
	if (!png_ptr) {
		fprintf(stderr, "unable to create png read struct\n");
		fclose(f);
		return NULL;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		fprintf(stderr, "unable to create png info struct\n");
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose(f);
		return NULL;
	}

	png_init_io(png_ptr, f);
	png_read_info(png_ptr, info_ptr);

	if (!png_get_IHDR(png_ptr, info_ptr, width, height, &bit_depth,
				&color_type, &interlace_type, NULL, NULL)) {
	}

	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	if (color_type == PNG_COLOR_TYPE_RGB ||
	    color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
		*is_rgb = 1;
		png_set_bgr(png_ptr);
	}

	png_read_update_info(png_ptr, info_ptr);

	rowbytes = png_get_rowbytes(png_ptr, info_ptr);

	image_data = (unsigned char *)malloc(rowbytes*(*height));
	if (!image_data) {
		fprintf(stderr, "out of memory\n");
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(f);
		return NULL;
	}
	
	row_pointers = calloc(*height, sizeof(png_bytep *));
	if (!row_pointers) {
		fprintf(stderr, "out of memory\n");
		free(image_data);
		fclose(f);
		return NULL;
	}

	for (i = 0;  i < *height;  ++i)
		row_pointers[i] = image_data + i*rowbytes;

	*bpr = rowbytes;
	*bpp = rowbytes/(*width);

	png_read_image(png_ptr, row_pointers);
	png_read_end(png_ptr, NULL);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	free(row_pointers);
	fclose(f);
	return image_data;
}

int main(int argc, char **argv)
{
	int fbdev;
	char *fb;
	unsigned long width, height, bpr;
	char rgb;
	unsigned int bpp;
	unsigned char *png_data;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	int ret = 0;
	int fname = 1;

	if (!have_framebuffer()) {
		fprintf(stderr,
			"error: this tool must be used with FB graphics\n");
		return 1;
	}

	if (argc < 2) {
		printf("usage: %s <device> <image file>\n", argv[0]);
		printf("usage: %s <image file>\n", argv[0]);
		return 0;
	}

	if (argc < 3)
		fbdev = open("/dev/fb0", O_RDWR|O_SYNC);
	else {
		fbdev = open(argv[1], O_RDWR|O_SYNC);
		fname++;
	}

	if (fbdev < 0) {
		perror("open FB device");
		return 1;
	}

	/* Get fixed screen information */
	if (ioctl(fbdev, FBIOGET_FSCREENINFO, &finfo)) {
		fprintf(stderr, "error reading fixed information.\n");
		ret = 2;
		goto out_dev;
	}

	/* Get variable screen information */
	if (ioctl(fbdev, FBIOGET_VSCREENINFO, &vinfo)) {
		fprintf(stderr, "error reading variable information.\n");
		ret = 3;
		goto out_dev;
	}
	vinfo.xoffset = vinfo.yoffset = 0;

	fb = (char *)mmap(0, finfo.smem_len,
			PROT_READ | PROT_WRITE, MAP_SHARED, fbdev, 0);
	if (fb == MAP_FAILED) {
		perror("mmap() failed");
		ret = 4;
		goto out_dev;
	}

	png_data = load_png(argv[fname], &width, &height, &bpr, &bpp, &rgb);
	if (!png_data)
		goto out_png;

	if (bpp<<3 != vinfo.bits_per_pixel)
		vinfo.bits_per_pixel = bpp<<3;

	if (rgb) {
		switch (vinfo.bits_per_pixel) {
			case 16:
				vinfo.blue.length = 5;
				vinfo.green.length = 6;
				vinfo.red.length = 5;
				vinfo.blue.offset = 0;
				vinfo.green.offset = 5;
				vinfo.red.offset = 11;
				break;
			case 24:
			case 32:
				vinfo.blue.length = 8;
				vinfo.green.length = 8;
				vinfo.red.length = 8;
				vinfo.blue.offset = 0;
				vinfo.green.offset = 8;
				vinfo.red.offset = 16;
				break;
		}
	} else {
		switch (vinfo.bits_per_pixel) {
			case 16:
				vinfo.blue.length = 5;
				vinfo.green.length = 6;
				vinfo.red.length = 5;
				vinfo.blue.offset = 11;
				vinfo.green.offset = 5;
				vinfo.red.offset = 0;
				break;
			case 24:
			case 32:
				vinfo.blue.length = 8;
				vinfo.green.length = 8;
				vinfo.red.length = 8;
				vinfo.blue.offset = 16;
				vinfo.green.offset = 8;
				vinfo.red.offset = 0;
				break;
		}
	}
	
	ioctl(fbdev, FBIOPUT_VSCREENINFO, &vinfo);
	draw(fb, png_data, &vinfo, width, height, bpr);

	free(png_data);
out_png:
	munmap(fb, finfo.smem_len);
out_dev:
	close(fbdev);
	return ret;
}
