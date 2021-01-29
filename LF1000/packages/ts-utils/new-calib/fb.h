// New Calibration

#define _FILE_OFFSET_BITS	64

#include <linux/fb.h>
#include <linux/lf1000/lf1000fb.h>

#define FB_NAME "/dev/fb0"

extern unsigned char* dc;
extern int vstride, hstride;
extern int vsize, hsize;

void cls ();
void inline unsafedot (int x, int y, long color)
{
	unsigned long *d = (unsigned long *)&dc[y*vstride+x*hstride];
	*d = color;
}
void inline dot (int x, int y, long color)
{
	if (x >= 0 && y>= 0 && x<hsize && y<vsize)
	{
		unsigned long *d = (unsigned long *)&dc[y*vstride+x*hstride];
		*d = color;
	}
}
void fatdot (int x, int y, int fatness, long color);
void fullfatdot (int x, int y, int fatness, long color);
void fullrounddot (int x, int y, int fatness2, long color);
void line (int x0, int y0, int x1, int y1, long color);
void fatline (int x0, int y0, int x1, int y1, int fatness0, int fatness1, long color);
void fullroundline (int x0, int y0, int x1, int y1, int fatness0, int fatness1, long color);
int init_fb ();
int exit_fb ();
