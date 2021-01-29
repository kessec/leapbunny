#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <string.h>
#include "fb.h"

static unsigned char *imgbuf = NULL;

void exit_drawtext ()
{
	if (imgbuf)
		free (imgbuf);
}

int init_drawtext ()
{
	struct stat statbuf;

	int img = open("/var/fonts/monotext8x16.rgb", O_RDONLY&~(O_CREAT));
	if(img < 0) {
		perror("failed to open image file");
		return 1;
	}

	/* read the image file to local buffer */
	if (fstat(img, &statbuf) < 0) {
		perror("fstat() failed");
		return 1;
	}
	int bufsize = statbuf.st_size;
	imgbuf = (unsigned char*)malloc(bufsize);
	if (imgbuf == NULL) {
		return 1;
	}
	if (read(img, imgbuf, bufsize) < 0) {
		perror("read() failed");
		return 1;
	}


	return 0;
}

/* write the image to the framebuffer */
void drawtext (int col, int row, const char *text)
{
	int i, j, k;
	unsigned char *s = (unsigned char*)imgbuf;
	unsigned char *d = (unsigned char*)dc;
	int ds = 128 * 3;
	int dd = vstride;
	for (i = 0; i < strlen(text); i++) {
		char c = text[i];
		int sc = c - 0x20;
		int sx = (sc % 0x10) * 8;
		int sy = (sc / 0x10) * 16;
		s = (unsigned char*)((int)imgbuf + (sy*128*3 + sx*3));
		d = (unsigned char*)((int)dc + (col+8*i)*hstride + row*vstride);
		for (j = 0; j < 16; j++) {
			for (k=0; k<8; k++) {
				*d++=~*s++;
				*d++=~*s++;
				*d++=~*s++;
				if (hstride == 4) *d++=0xff;
			}
			s += ds - 3*8;
			d += dd - hstride*8;
		}	
	}
}
