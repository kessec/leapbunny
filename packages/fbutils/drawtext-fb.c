/* Example for writing mono-spaced text glyphs to the frame buffer */
/* Each text glyph is 8x16 RGB pixels mapped to ASCII char codes 0x20..0x80 in
 * rows of 0x10 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include "check-fb.h"

int main(int argc, char **argv)
{
	int fbdev;
	unsigned long bufsize;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	int img = -1;
	int fb = -1;
	int hstride = -1;
	unsigned int row = 0, col = 0;
	unsigned char* imgbuf = NULL;
	struct stat statbuf;
	int i,j,k,ds,dd;
	unsigned char* s;
	unsigned char* d;
	const char *text = "Hello World";
	enum mode_type { normal=0, invert=1 } mode=normal;
	int ret = 0;

	if (!have_framebuffer()) {
		fprintf(stderr,
			"error: this tool must be used with FB graphics\n");
		return 1;
	}

	if (argc < 4) {
		printf("Draw mono-spaced text glyphs to a frame buffer.\n"
			   "\tusage: %s <device> <file> <text> [<row> [<col> [<mode>]]]\n"
			   "\t(example: %s /dev/fb0 /var/fonts/monotext8x16.rgb \"quoted text\" 10 5 1)\n"
			   "\tmode=0 is normal; mode=1 is inverse\n", argv[0],
			   argv[0]);
		return 0;
	}

	if (argc >= 5) {
		if (sscanf(argv[4], "%u", &row) != 1) {
			printf("invalid argument\n");
			return 1;
		}
	}

	if (argc >= 6) {
		if (sscanf(argv[5], "%u", &col) != 1) {
			printf("invalid argument\n");
			return 1;
		}
	}

	if (argc >= 7) {
		if (sscanf(argv[6], "%u", &mode) != 1) {
			printf("invalid argument\n");
			return 1;
		}
	}

	fbdev = open(argv[1], O_RDWR|O_SYNC);
	if (fbdev < 0) {
		perror("open device");
		return 2;
	}

	/* Get fixed screen information */
	if (ioctl(fbdev, FBIOGET_FSCREENINFO, &finfo)) {
		printf("Error reading fixed information.\n");
		ret = 2;
		goto out_dev;
	}

	/* Get variable screen information */
	if (ioctl(fbdev, FBIOGET_VSCREENINFO, &vinfo)) {
		printf("Error reading variable information.\n");
		ret = 3;
		goto out_dev;
	}
	vinfo.xoffset = vinfo.yoffset = 0;
	if (ioctl(fbdev, FBIOPUT_VSCREENINFO, &vinfo)) {
		printf("Error writing variable information.\n");
		ret = 3;
		goto out_dev;
	}

	hstride = vinfo.bits_per_pixel>>3;

	fb = (int)mmap(0, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED,
			fbdev, 0);
	if (fb < 0) {
		perror("mmap() failed");
		ret = 4;
		goto out_dev;
	}

	img = open(argv[2], O_RDONLY&~(O_CREAT));
	if (img < 0) {
		perror("open image file");
		ret = 5;
		goto out_open;
	}

	if (fstat(img, &statbuf) < 0) {
		perror("fstat");
		ret = 6;
		goto out_open;
	}

	bufsize = (statbuf.st_size < finfo.smem_len) ?
		statbuf.st_size : finfo.smem_len;
	imgbuf = (unsigned char*)malloc(bufsize);
	if (imgbuf == NULL) {
		perror("malloc");
		ret = 7;
		goto out_open;
	}

	if (read(img, imgbuf, bufsize) < 0) {
		perror("read");
		ret = 8;
		goto out_read;
	}

	/* write the image to the framebuffer */
	s = (unsigned char*)imgbuf;
	d = (unsigned char*)fb;
	ds = 128 * 3;
	dd = vinfo.xres * hstride;
	text = argv[3];
	for (i = 0; i < strlen(text); i++) {
		char c = text[i];
		int sc = c - 0x20;
		int sx = (sc % 0x10) * 8;
		int sy = (sc / 0x10) * 16;
		s = (unsigned char*)((int)imgbuf + (sy*128*3 + sx*3));
		d = (unsigned char*)((int)fb + (i+col)*8*hstride +
				     dd*16*row);
		for (j = 0; j < 16; j++) {
			if (mode == normal) {
				for (k=0; k<8; k++) {
					*d++=*s++;
					*d++=*s++;
					*d++=*s++;
					if (hstride == 4) *d++=0xff;
				}
			} else { /* mode == inverse */
				for (k=0; k<8; k++) {
					*d++=~*s++;
					*d++=~*s++;
					*d++=~*s++;
					if (hstride == 4) *d++=0xff;
				}
			}
			s += ds - 3*8;
			d += dd - hstride*8;
		}	
	}

out_read:
	free(imgbuf);
out_open:
	munmap((char *)fb, finfo.smem_len);
out_dev:
	close(fbdev);
	return ret;	
}
