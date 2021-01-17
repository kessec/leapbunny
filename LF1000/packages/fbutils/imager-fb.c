#define _FILE_OFFSET_BITS	64	// for correct off_t type

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <string.h>
#include <linux/fb.h>

/* Added for png support */
#include "readpng.h"
#include "formats.h"

int main(int argc, char **argv)
{
	int fbdev = -1;
	int img = -1;
	void *fb = NULL;
	unsigned char* imgbuf = NULL;
	FILE *pngfile = NULL;
	int fbsize, bufsize;
	struct stat statbuf;
	unsigned long width, height, bpr;
	int bpp;
	unsigned char *png_data = NULL;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;

	if(argc < 2) {
		printf("usage: %s <image file>\n", argv[0]);
		return 0;
	}

	fbdev = open("/dev/fb0", O_RDWR|O_SYNC);
	if(fbdev < 0) {
		perror("failed to open fb device");
		return 1;
	}

	/* Get fixed screen information */
	if (ioctl(fbdev, FBIOGET_FSCREENINFO, &finfo)) {
		printf("Error reading fixed information.\n");
		exit(2);
	}

	/* Get variable screen information */
	if (ioctl(fbdev, FBIOGET_VSCREENINFO, &vinfo)) {
		printf("Error reading variable information.\n");
		exit(3);
	}

	/* Figure out the size of the screen in bytes */
	fbsize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

	fb = mmap(0, fbsize, PROT_READ | PROT_WRITE, MAP_SHARED, fbdev, 0);
	if(fb == MAP_FAILED) {
		perror("mmap() failed");
		goto cleanup;
	}

	/* The file is either a raw rgb or png. */
	img = open(argv[1], O_RDONLY & ~(O_CREAT));
	if(img < 0) {
		perror("failed to open image file");
		close(fbdev);
		return 1;
	}

	if (!(pngfile = fopen(argv[1], "rb"))) {
		perror("can't open intput file.");
		return 1;
	}

	if (readpng_init(pngfile, &width, &height) == 0) {
		/* We have a png. Decode it. */
		png_data = readpng_get_image(2.2, &bpp, &bpr);
		if (!png_data) {
			fprintf(stderr, "unable to decode PNG image\n");
			goto cleanup;
		}	
		
		/* Check image format */
		if(vinfo.yres != height || vinfo.xres != width) {
			fprintf(stderr,
				"Image dimensions don't match screen\n");
			goto cleanup;
		}
		
		memcpy(fb, png_data, height*width*bpp);
		readpng_cleanup(FALSE);

	} else {

		/* read the image file to local buffer */
		if (fstat(img, &statbuf) < 0) {
			perror("fstat() failed");
			goto cleanup;
		}
		bufsize = (statbuf.st_size < fbsize) ? statbuf.st_size : fbsize;
		imgbuf = (unsigned char*)malloc(bufsize);
		if (imgbuf == NULL) {
			perror("malloc() failed");
			goto cleanup;
		}
		if (read(img, imgbuf, bufsize) < 0) {
			perror("read() failed");
			goto cleanup;
		}
		/* write the image to the framebuffer */
		memcpy((void*)fb, imgbuf, bufsize);
	}

cleanup:
	if (imgbuf != NULL) free(imgbuf);
	if (fb != NULL) munmap(fb, fbsize);
	if (img >= 0) close(img);
	if (fbdev >= 0) close(fbdev);
	if (png_data) free(png_data);
	if (pngfile) fclose(pngfile);
	return 0;
}
