#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include "fb.h"

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
int hstride, vstride;
int hsize, vsize;
void *fb = (void *)-1;
int fbdev;
unsigned char *dc;

void cls ()
{
	memset (dc, 0xff, hstride*hsize*vsize);
}

void fatdot (int x, int y, int fatness, long color)
{
	int i;
	dot (x, y, color);
	for (i=1; i<=fatness; i++)
		dot (x-i, y, color), dot (x+i, y, color);
}

void fullfatdot (int x, int y, int fatness, long color)
{
	int i, j, f=fatness-1;
	for (i=x-f; i<=x+f; i++)
		for (j=y-f; j<=y+f; j++)
			dot (i, j, color);
}

void fullrounddot (int x, int y, int fatness2, long color)
{
	int i, j, f;
	for (f=0; f*f<fatness2; f++);
	for (i=-f; i<=f; i++)
		for (j=-f; j<=f; j++)
			if (i*i+j*j<=fatness2)
				dot (i+x, j+y, color);
}

void line (int x0, int y0, int x1, int y1, long color)
{
	int sx=1, sy=1;
	int dx=x1-x0;
	int ax=dx; if (ax<0) { sx=-1; ax=-ax; }
	int dy=y1-y0;
	int ay=dy; if (ay<0) { sy=-1; ay=-ay; }
	if (ax>=ay && ax>0)
	{
		int i;
		int d=(65536*dy)/ax;
		int s=65536*y0;
		for (i=0; i<=ax; i++)
		{
			int ss = s>>16;
			dot (x0+sx*i, ss, color);
			s += d;
		}
	}
	else if (ax<ay && ay>0)
	{
		int i;
		int d=(65536*dx)/ay;
		int s=65536*x0;
		for (i=0; i<=ay; i++)
		{
			int ss = s>>16;
			dot (ss, y0+sy*i, color);
			s += d;
		}
	}
}

void fatline (int x0, int y0, int x1, int y1, int fatness0, int fatness1, long color)
{
	int sx=1, sy=1;
	int dx=x1-x0;
	int ax=dx; if (ax<0) { sx=-1; ax=-ax; }
	int dy=y1-y0;
	int ay=dy; if (ay<0) { sy=-1; ay=-ay; }
	int fatness = 65536*fatness0;
	if (ax>=ay && ax>0)
	{
		int i;
		int d=(65536*dy)/ax;
		int s=65536*y0;
		int df=65536*(fatness1-fatness0)/ax;
		for (i=0; i<=ax; i++)
		{
			int ss = s>>16;
			fatdot (x0+sx*i, ss, fatness>>16, color);
			s += d;
			fatness += df;
		}
	}
	else if (ax<ay && ay>0)
	{
		int i;
		int d=(65536*dx)/ay;
		int s=65536*x0;
		int df=65536*(fatness1-fatness0)/ay;
		for (i=0; i<=ay; i++)
		{
			int ss = s>>16;
			fatdot (ss, y0+sy*i, fatness>>16, color);
			s += d;
			fatness += df;
		}
	}
}

void fullroundline (int x0, int y0, int x1, int y1, int fatness0, int fatness1, long color)
{
	int sx=1, sy=1;
	int dx=x1-x0;
	int ax=dx; if (ax<0) { sx=-1; ax=-ax; }
	int dy=y1-y0;
	int ay=dy; if (ay<0) { sy=-1; ay=-ay; }
	int fatness = 65536*fatness0;
	if (ax>=ay && ax>0)
	{
		int i;
		int d=(65536*dy)/ax;
		int s=65536*y0;
		int df=65536*(fatness1-fatness0)/ax;
		for (i=0; i<=ax; i++)
		{
			int f = fatness>>8;
			int ss = s>>16;
			fullrounddot (x0+sx*i, ss, (f*f)>>16, color);
			s += d;
			fatness += df;
		}
	}
	else if (ax<ay && ay>0)
	{
		int i;
		int d=(65536*dx)/ay;
		int s=65536*x0;
		int df=65536*(fatness1-fatness0)/ay;
		for (i=0; i<=ay; i++)
		{
			int f = fatness>>8;
			int ss = s>>16;
			fullrounddot (ss, y0+sy*i, (f*f)>>16, color);
			s += d;
			fatness += df;
		}
	}
}

int init_fb ()
{
	// Set up frame buffer
	fbdev = open("/dev/fb0", O_RDWR|O_SYNC);
	if(fbdev < 0)
	{
		perror("failed to open fb device");
		return 1;
	}

	// Ask for screen res in hacky way
	FILE *p = fopen ("/sys/devices/platform/lf1000-dpc/xres", "r");
	if (!p)
	{
		perror("failed to open /sys/devices/platform/lf1000-dpc/xres");
		return 1;
	}
	fscanf (p, "%d", &hsize);
	fclose (p);
	p = fopen ("/sys/devices/platform/lf1000-dpc/yres", "r");
	if (!p)
	{
		perror("failed to open /sys/devices/platform/lf1000-dpc/yres");
		return 1;
	}
	fscanf (p, "%d", &vsize);
	fclose (p);

	/* Get fixed screen information */
	if (ioctl(fbdev, FBIOGET_FSCREENINFO, &finfo)) {
		perror("error reading fixed information.");
		return 1;
	}

	/* Get variable screen information */
	if (ioctl(fbdev, FBIOGET_VSCREENINFO, &vinfo)) {
		perror("error reading variable information.");
		return 1;
	}

	// Force 32 bits per pixel
	vinfo.bits_per_pixel = 32;

	//	printf ("fixed: line_length=%d mmio_len=%d smem_len=%d\n",
	//		finfo.line_length, finfo.mmio_len, finfo.smem_len);

	hstride = vinfo.bits_per_pixel>>3;
	vstride = hsize * hstride; // finfo.line_length;
	vinfo.xres = hsize;
	vinfo.yres = vsize;
	vinfo.width = hsize;
	vinfo.height = vsize;
	vinfo.blue.offset=0;
	vinfo.green.offset=8;
	vinfo.red.offset=16;
	// printf ("vari: res(%d,%d) vres(%d,%d) offset(%d,%d) bbp=%d w,h=%d,%d\n",
	// 	vinfo.xres, vinfo.yres, 
	// 	vinfo.xres_virtual, vinfo.yres_virtual, 
	// 	vinfo.xoffset, vinfo.yoffset, 
	// 	vinfo.bits_per_pixel,
	// 	vinfo.width, vinfo.height);

	if (ioctl(fbdev, FBIOPUT_VSCREENINFO, &vinfo))
	{
		perror("error setting variable info");
		return 1;
	}
	

	struct lf1000fb_position_cmd cmd;
	cmd.left = cmd.top = 0;
	cmd.right=hsize;
	cmd.bottom=vsize;
	if (ioctl(fbdev, LF1000FB_IOCSPOSTION, &cmd))
	{
		perror("error setting lf1000 position");
		return 1;
	}

	// Blend off
	struct lf1000fb_blend_cmd bcmd;
	bcmd.enable = 0;
	bcmd.alpha = 0;
	if (ioctl(fbdev, LF1000FB_IOCSALPHA, &bcmd))
	{
		perror("error setting lf1000 blend");
		return 1;
	}

	fb = (char *)mmap(0, finfo.smem_len,
			  PROT_READ | PROT_WRITE, MAP_SHARED, fbdev, 0);
	if (fb == MAP_FAILED) {
		perror("mmap() failed");
		return 1;
	}

	// printf ("fb=%p hstride=%d vstride=%d w=%d h=%d\n", 
	// 	fb, hstride, vstride, hsize, vsize);

 	dc = (unsigned char*)fb;
	return 0;
}

int exit_fb ()
{
	munmap (fb, finfo.smem_len);
	close (fbdev);
}

