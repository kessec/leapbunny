/* Example for writing mono-spaced text glyphs to the frame buffer */
/* Each text glyph is 8x16 RGB pixels mapped to ASCII char codes 0x20..0x80 in rows of 0x10 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#include <linux/lf1000/mlc_ioctl.h>

#include "check-fb.h"

int main(int argc, char **argv)
{
	int layer = -1;
	int img = -1;
	int fb = -1;
	int hstride = -1;
	int row = 0;
	int col = 0;
	unsigned char* imgbuf = NULL;
	int base, fbsize, bufsize;
	struct stat statbuf;
	int i,j,k,ds,dd;
	unsigned char* s;
	unsigned char* d;
	const char * text = "Hello World";
	enum mode_type { normal=0, invert=1 } mode=normal;

	if (have_framebuffer()) {
		printf("Error: this tool isn't compatible with FB graphics\n");
		return 1;
	}

	if(argc < 4) {
		printf("Draw mono-spaced text glyphs to a frame buffer.\n"
			   "\tusage: drawtext <layer> <rgb_file> <text> [<row> [<col> [<mode>]]]\n"
			   "\t(example: drawtext /dev/layer0 /test/monotext8x16.rgb \"quoted text\" 10 5 1)\n"
			   "\tmode=0 is normal; mode=1 is inverse\n");
		return 0;
	}

	/* Get line number, if specified */
	if (argc >= 5)
		row = atoi(argv[4]);
	if (argc >= 6)
		col = atoi(argv[5]);
	if (argc >= 7)
		mode = atoi(argv[6]);


	layer = open(argv[1], O_RDWR|O_SYNC);
	if(layer < 0) {
		perror("failed to open layer device");
		return 1;
	}

	/* get the base address for this layer's frame buffer */
	base = ioctl(layer, MLC_IOCQADDRESS, 0);
	if(base == -EFAULT) {
		perror("get_address ioctl failed");
		goto cleanup;
	}

	/* figure out the size of the frame buffer */
	fbsize = ioctl(layer, MLC_IOCQFBSIZE, 0);
	if(fbsize < 0) {
		perror("get_fbsize ioctl failed");
		goto cleanup;
	}

	hstride = ioctl(layer, MLC_IOCQHSTRIDE, 0);
	/* printf ("hstride=%d\n", hstride); */
	if(hstride < 0) {
		perror("get_hstride ioctl failed");
		goto cleanup;
	}

	fb = (int)mmap(0, fbsize, PROT_READ | PROT_WRITE, MAP_SHARED, layer, 
				   (unsigned int)base);
	if(fb < 0) {
		perror("mmap() failed");
		goto cleanup;
	}

	/* The file is either a raw rgb or png. */
	img = open(argv[2], O_RDONLY&~(O_CREAT));
	if(img < 0) {
		perror("failed to open image file");
		close(layer);
		return 1;
	}

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
	s = (unsigned char*)imgbuf;
	d = (unsigned char*)fb;
	ds = 128 * 3;
	dd = 320 * hstride;
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
			if (mode == normal)
			{
				for (k=0; k<8; k++) {
					*d++=*s++;
					*d++=*s++;
					*d++=*s++;
					if (hstride == 4) *d++=0xff;
				}
			}
			else /* mode == inverse */
			{
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
		
cleanup:
	if (imgbuf != NULL) free(imgbuf);
	if (fb >= 0) close(fb);
	if (img >= 0) close(img);
	if (layer >= 0) close(layer);
	return 0;
}
