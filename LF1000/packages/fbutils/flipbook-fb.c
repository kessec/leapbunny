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

void draw_frame(unsigned char *s, int work, int bg,
		struct fb_var_screeninfo *vinfo)
{
	unsigned char *d = (unsigned char *)work;
	int x, y, a;
	unsigned char r0,g0,b0;
	unsigned char r1,g1,b1;

	for (y = 0; y < vinfo->yres; y++) {
		for (x = 0; x < vinfo->xres; x++) {
			if (s[4]==0) { /* transparent */
				s += 4;
				d += 4;
			} else { /* blended */
				r0 = *s++;
				g0 = *s++;
				b0 = *s++;
				a = *s++;
				r1 = *d++;
				g1 = *d++;
				b1 = *d--;
				d--;
				*d++ = ((r0 * a)+(r1 * (256-a))) >> 8;
				*d++ = ((g0 * a)+(g1 * (256-a))) >> 8;
				*d++ = ((b0 * a)+(b1 * (256-a))) >> 8;
				d++;
			}
		}
	}
}

#if 0
void draw_frame_portrait(unsigned char *s, int work, int bg,
			 struct fb_var_screeninfo *vinfo)
{
	unsigned char *d = (unsigned char *)work, *d1;
	int x, y, a;
	unsigned char r0,g0,b0;
	unsigned char r1,g1,b1;

	int xstride = (sizeof (long))*(vinfo->xres);
	int ystride = (sizeof (long))*(-1);
	d += (sizeof (long))*(xstride-1);
	for (y = 0; y < vinfo->yres; y++) {
		d1 = d + ystride;
		for (x = 0; x < vinfo->xres; x++) {
			if (s[4]==0) { /* transparent */
				s += 4;
				d += xstride;
			} else { /* blended */
				r0 = *s++;
				g0 = *s++;
				b0 = *s++;
				a = *s++;
				r1 = *d++;
				g1 = *d++;
				b1 = *d--;
				d--;
				*d++ = ((r0 * a)+(r1 * (256-a))) >> 8;
				*d++ = ((g0 * a)+(g1 * (256-a))) >> 8;
				*d++ = ((b0 * a)+(b1 * (256-a))) >> 8;
				d++;
				d+= xstride-(sizeof (long));
			}
		}
		d = d1;
	}
}
#endif

int main(int argc, char **argv)
{
	int fbdev;
	int fb;
	int i, j;
	unsigned long width, height, bpr;
	int bpp;
	unsigned int hstride, vstride;
	int ORDER;
	unsigned char *png_data[MAX_FRAMES];
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
	for (i=1; i<=MAX_FRAMES; i++) {
		char name[128];
		snprintf(name, 127, "%s/Particle_%04d.png", argv[1], i*skip);
		if (!(pngfile = fopen(name, "rb"))) {
			if (i>1)
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
		png_data[i-1] = readpng_get_image(2.2, &bpp, &bpr);
		if (!png_data[i-1]) {
			fprintf(stderr, "unable to decode \"%s\"\n",
					name);
			ret = 9;
			fclose(pngfile);
			goto out_open;
		}
		fclose(pngfile);
	}

 start_drawing:
	ORDER=i-1;
	if (bpp<<3 != vinfo.bits_per_pixel) {
		vinfo.bits_per_pixel = bpp<<3;
		if (ioctl(fbdev, FBIOPUT_VSCREENINFO, &vinfo)) {
			perror("set screen info");
			ret = 10;
			goto out_open;
		}
	}
		
	/* draw the bezier */
	i = 0;
	while (1) {
		const int frames = 60;
		struct timeval t0, t1;
		gettimeofday(&t0, 0);
		for (j = 0; j < frames; j++) {
			scrcpy(work, bg, &vinfo); /* clear out work space */
			draw_frame(png_data[i++ % ORDER], work, fb, &vinfo);
			scrcpy(fb, work, &vinfo); /* refresh screen */
			usleep(1000000/rate);
		}

		if (argc > 4) {
			gettimeofday(&t1, 0);
			float delta = t1.tv_sec - t0.tv_sec +
				1e-6*(t1.tv_usec - t0.tv_usec);
			float fps = frames / delta;
			printf("%5.2f fps\n", fps);
		}
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
