#define _FILE_OFFSET_BITS	64

#include <stdint.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <unistd.h>
#include <poll.h>

#include <tslib.h>

#define TS_NAME	"LF1000 touchscreen interface"
#define KB_NAME	"LF1000 Keyboard"

char ts_dev_name[20];

#include "fb.h"
#include "drawtext.h"
#include "OpenInput.h"

#define CLSIZE	256
long colorlut[CLSIZE];

#include <math.h>
void fill_color_lut ()
{
	int i;
	for (i=0; i<CLSIZE; i++)
	{
		double r=(127*sin(i*.01)+128)*i/CLSIZE;
		double g=(127*sin(i*.025)+128)*i/CLSIZE;
		double b=(127*sin(i*.07)+128)*i/CLSIZE;
		r = sqrt (r*256);
		g = sqrt (g*256);
		b = sqrt (b*256);
		long color = (((int)r)<<16) |
			(((int)g)<<8) | 
			(((int)b)<<0);
		colorlut[i] = color;
	}
}

#define NTHUMBS 10
#define THUMBRAD 10
#define THUMBCEN (THUMBRAD+1)
#define THUMBDIA (THUMBRAD+THUMBCEN)

int thumblut[NTHUMBS][THUMBDIA+1][THUMBDIA+1];

void fill_thumb_lut ()
{
	int i;
	double x, y;
	int ix, iy;
	for (i=0; i<NTHUMBS; i++)
	{
		int r=i+1;
		for (ix=0, x=-THUMBRAD; x<=THUMBRAD; ix++, x++)
			for (iy=0, y=-THUMBRAD; y<=THUMBRAD; iy++, y++)
			{
				if (x*x+y*y > r*r)
					thumblut[i][ix][iy] = 0;
				else
					thumblut[i][ix][iy] = (int) sqrt (r*r-x*x-y*y);
			}
		for (ix=0; ix<=THUMBDIA; ix++)
		{
			for (iy=0; iy<=THUMBDIA; iy++)
				printf ("%2d ", thumblut[i][ix][iy]);
			printf ("\n");
		}
		printf ("\n");
	}
}

int main (int argc, char **argv)
{
	int load_x = 0;
	if (init_fb ())
		return 1;
	if (init_drawtext())
		return 1;

	// Clear screen
	cls ();

	// Keyboard
	int kb = open_input_device(KB_NAME);
	if (kb < 0)
	{
		perror("Can't find " KB_NAME);
		return 1;
	}
	struct pollfd polldat[1] = {kb, POLLIN };

	// TSLIB
	if (find_input_device (TS_NAME, ts_dev_name) < 0)
	{
		perror ("Can't find " TS_NAME);
		return 1;
	}
	struct tsdev *tsl = ts_open(ts_dev_name, 1);
	if (!tsl) {
		perror("ts_open");
		return 1;
	}

	if (ts_config(tsl)) {
		perror("ts_config");
		return 1;
	}
	int ts = ts_fd (tsl);
	if (ts < 0)
	{
		perror("failed to open ts device");
		return 1;
	}


	int s0[hsize][vsize];
	int s1[hsize][vsize];

	memset (s0, 0, sizeof(s0));
	memset (s1, 0, sizeof(s1));

	fill_color_lut ();
	fill_thumb_lut ();

	// The main event loop
	while (1) {
		struct ts_sample samp[64];
		struct input_event ev[64];
		struct timeval tv0, tv1;
		int i;
		
		gettimeofday (&tv0, NULL);
		// tslib have something for us?  If so, draw it
		int ret = ts_read(tsl, samp, 64);
		for (i=0; i<ret; i++)
		{
			int p=samp[i].pressure/10;
			if (p>NTHUMBS-1) p=NTHUMBS-1;
			int x0=samp[i].x-THUMBRAD;
			int y0=samp[i].y-THUMBRAD;
			int x, y;
			int ix, iy;
			for (ix=0; ix<=THUMBDIA; ix++)
			{
				x=x0+ix;
				for (iy=0; iy<=THUMBDIA; iy++)
				{
					y=y0+iy;
					if (x>=0 && x<hsize && y>=0 && y<vsize)
					{
						s0[x][y] += 10*thumblut[p][ix][iy];
					}
				}
			}
			// s0[x][y] += samp[i].pressure * samp[i].pressure;
		}
		
		// input event system: poll keyboard[1]
		if (poll (polldat, 1, 0) > 0)
		{
			if (polldat[0].revents) // Keyboard
			{
				int rd = read(kb, ev, sizeof(struct input_event));
				if (rd == sizeof (struct input_event))
				{
					if (ev[0].type == EV_KEY && 
					    ev[0].value==1)
					{
						if (ev[0].code == KEY_A || ev[0].code == KEY_ESC)
						{
							cls ();
						}
					}
				}
			}
		}

		// Convolution
		int x,y;
#if 1
		for (y=0; y<vsize; y++)
		{
			for (x=1; x<hsize-1; x++)
			{
				s1[x][y] = (s0[x-1][y]+2*s0[x][y]+s0[x+1][y])/4;
			}
		}
		for (x=0; x<hsize-0; x++)
		{
			for (y=1; y<vsize-1; y++)
			{
				s0[x][y] = (s1[x][y-1]+2*s1[x][y]+s1[x][y+1])/4;
			}
		}
#endif
		// Render
		long *p = (long *)dc;
		for (y=0; y<vsize; y++)
		{
			for (x=0; x<hsize; x++)
			{
				int c=s0[x][y];
				if (c>255) c=255;
				*p++ = colorlut[c];
			}
		}
		gettimeofday (&tv1, NULL);
		static int x9=0;
		if (!(++x9 & 31))
		{
			float d=tv1.tv_sec-tv0.tv_sec+
				1e-6*(tv1.tv_usec-tv0.tv_usec);
			printf ("FPS=%.0f\n", 1/d);
		}
	}
}

