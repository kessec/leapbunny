/* Draw pngs as a flip-book movie */
/* Code generally lifted from Dave's drawtext */

#define MAX_FRAMES	500

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include "readpng.h"
#include "check-fb.h"

void scrcpy(int dst, int src, struct fb_var_screeninfo *vinfo)
{
	memcpy((char *)dst, (char *)src,
			vinfo->xres*vinfo->yres*4);
}

struct spark_data
{
	float x, y;
	float vx, vy;
	// unsigned long color;
	long state;
} *spark;

struct dot
{
	int x, y;
	unsigned char r,g,b,a;
};


struct interesting
{
	int length;
	struct dot *dots;
};

struct interesting *get_interesting (unsigned char *data, int width, int height)
{
	int x, y;
	int count = 0;
	unsigned char *s;

	/* Count non-transparent pixels */
	s = data;
	for (y = 0; y < width; y++) {
		for (x = 0; x < height; x++) {
			if (s[3]!=0)
				count++;
			s += 4;
		}
	}

	/* Print out count */
	printf ("%d interesting pixels in %dx%d image\n", count, width, height);

	struct interesting *r = new struct interesting;
	r->length = count;
	r->dots = NULL;
	if (count==0)
		return r;

	/* Make an array of x,y,rgba */
	r->dots = new struct dot[count];
	count = 0;
	s = data;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			if (s[3]!=0)
			{
				r->dots[count].x = x;
				r->dots[count].y = y;
				r->dots[count].r = s[0];
				r->dots[count].g = s[1];
				r->dots[count].b = s[2];
				r->dots[count].a = s[3];
				count++;
			}
			s += 4;
		}
	}
	return r;
}

void draw_frame(struct interesting *data, int work,
		struct fb_var_screeninfo *vinfo)
{
	unsigned char *d = (unsigned char *)work;
	int a, i;
	unsigned char r0,g0,b0;
	unsigned char r1,g1,b1;

	for (i=0; i<data->length; i++)
	{
		struct dot *t = &data->dots[i];
		d = (unsigned char *)work + (t->x + t->y * vinfo->xres)*4;
		r0 = t->r;
		g0 = t->g;
		b0 = t->b;
		a = t->a;
		r1 = *d++;
		g1 = *d++;
		b1 = *d--;
		d--;
		*d++ = ((r0 * a)+(r1 * (256-a))) >> 8;
		*d++ = ((g0 * a)+(g1 * (256-a))) >> 8;
		*d++ = ((b0 * a)+(b1 * (256-a))) >> 8;
	}
}

int main(int argc, char **argv)
{
	int fbdev;
	int fb;
	int i;
	unsigned long width, height, bpr;
	int bpp;
	unsigned int hstride, vstride;
	int ORDER;
	struct interesting *in[MAX_FRAMES];
	FILE *pngfile;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	int bg;
	int work;
	int ret = 0;
	int skip = 1;
	int rate = 20;

	if (!have_framebuffer()) {
		printf("Error: this tool must be used with FB graphics\n");
		return 1;
	}

	if (argc < 2) {
		fprintf(stderr, "Usage: %s DIRECTORY [skip [rate]]\n", argv[0]);
		return 0;
	}

	if (argc > 2)
	{
		skip = atoi (argv[2]);
	}

	if (argc > 3)
	{
		rate = atoi (argv[3]);
	}

	fbdev = open("/dev/fb0", O_RDWR|O_SYNC);
	if (fbdev < 0) {
		perror("open device");
		return 1;
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

	/* Figure out the size of the screen in bytes */
	hstride = 4;
	vstride = vinfo.xres * hstride;

	fb = (int)mmap(0, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED,
			fbdev, 0);
	if (fb < 0) {
		perror("mmap");
		ret = 4;
		goto out_dev;
	}

	bg = (int)malloc(vinfo.yres*vstride);
	if (!bg) {
		perror("bg malloc");
		ret = 5;
		goto out_bg;
	}

	work = (int)malloc(vinfo.yres*vstride);
	if (!work) {
		perror("work malloc");
		ret = 6;
		goto out_work;
	}

	/* save the background */
	scrcpy(bg, fb, &vinfo);

	/* open PNGs, assume they all have the same BPP */
	for (i=0; i<MAX_FRAMES; i++) {
		char name[128];
		snprintf(name, 127, "%s/Particle_%04d.png", argv[1], 1+i*skip);
		if (!(pngfile = fopen(name, "rb"))) {
			if (i>0)
			{
				goto start_drawing;
			}
			perror("open intput file");
			ret = 7;
			goto out_open;
		}

		ret = readpng_init(pngfile, &width, &height);
		if (ret) {
			fprintf(stderr, "can't read from \"%s\"\n", name);
			ret = 8;
			fclose(pngfile);
			goto out_open;
		}

		/* We have a png. Decode it. */
		uch *data = readpng_get_image(2.2, &bpp, &bpr);
		fclose(pngfile);
		if (!data) {
			fprintf(stderr, "unable to decode \"%s\"\n", name);
			ret = 9;
			goto out_open;
		}

		/* Collect the data that matters */
		in[i] = get_interesting(data, width, height);

		readpng_cleanup (1);
	}

 start_drawing:
	if (bpp<<3 != vinfo.bits_per_pixel) {
		vinfo.bits_per_pixel = bpp<<3;
		if (ioctl(fbdev, FBIOPUT_VSCREENINFO, &vinfo)) {
			perror("set screen info");
			ret = 10;
			goto out_open;
		}
	}
		
	/* draw the bezier */
	ORDER=i;
	i = 0;
	while (1) {
		scrcpy(fb, bg, &vinfo); /* clear out work space */
		draw_frame(in[i++ % ORDER], fb, &vinfo);
		// scrcpy(fb, work, &vinfo); /* refresh screen */
		usleep(1000000/rate);
	}

out_open:
	free((char *)work);
out_work:
	free((char *)bg);
out_bg:
	munmap((char *)fb, finfo.smem_len);
out_dev:
	close(fbdev);
	return ret;
}
