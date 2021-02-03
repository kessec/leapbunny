// MPH: How fast do we draw (for tuning tslib)
//
// bins:
//    Mode around 25, fastest around 50,  wild > 100
//    10 20 30 40 50 60 70 80 90 100


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
#include <inttypes.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <unistd.h>
#include <poll.h>

#include <tslib.h>
#include <math.h>

#define TS_NAME	"LF1000 touchscreen interface"
#define KB_NAME	"LF1000 Keyboard"

char ts_dev_name[20];

#include "fb.h"
#include "drawtext.h"
#include "OpenInput.h"

int main (int argc, char **argv)
{
	printf ("%s: MPH\n", argv[0]);
	printf ("Press A to clear screen, B to exit\n");
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
	// tsraw
	if (find_input_device (TS_NAME, ts_dev_name) < 0)
	{
		perror ("Can't find " TS_NAME);
		return 1;
	}
	int tsr = open (ts_dev_name, O_RDONLY);
	if (tsr < 0)
	{
		perror(ts_dev_name);
		return 1;
	}

	struct pollfd polldat[2] = {{kb, POLLIN}, {tsr, POLLIN} };

	// TSLIB
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

#define HN 10
#define HW 10
#define HMAX (HN*NW)
	int hr[HN], ht[HN];
	memset (hr, 0, sizeof(hr));
	memset (ht, 0, sizeof(ht));

	// The main event loop
	while (1) {
		static int tlx=-1, tly=-1, rlx=-1, rly=-1;
		struct ts_sample samp[64];
		struct input_event ev[64];
		int i;
		static double avgdr = 0;
		static double avgdt = 0;
		char buf[100];

		// tslib have something for us?  If so, draw it
		int ret = ts_read(tsl, samp, 64);
		for (i=0; i<ret; i++)
		{
			int color=0xff0000;
			if (tlx != -1 && tly != -1)
			{
				// Existing
				line (tlx, tly, samp[i].x, samp[i].y, color);
				double dx=(tlx-samp[i].x);
				double dy=(tly-samp[i].y);
				double d = sqrt (dx*dx+dy*dy);
				avgdt = .98*avgdt + .02*d;
				printf ("\t\tt %4.1f %4.1f\n", d, avgdt);
				drawtext (0,0, buf);
				int h=(int)d/HW;
				if (h<0) h=0;
				if (h>HN-1) h=HN-1;
				ht[h]++;
				int xx=ht[h];
				int yy=h*vsize/HN;
				line (xx,yy,xx,yy+vsize/HN-1, color);
			}
			tlx = samp[i].x;
			tly = samp[i].y;
			fullfatdot (tlx, tly, 2, color);
			if (!samp[i].pressure)
				tlx = tly = -1;
		}
		
		// input event system: poll keyboard[1]
		if (poll (polldat, 2, 0) > 0)
		{
			if (polldat[0].revents) // Keyboard
			{
				int rd = read(kb, ev, sizeof(struct input_event));
				if (rd == sizeof (struct input_event))
				{
					if (ev[0].type == EV_KEY && 
					    ev[0].value==1)
					{
						if (ev[0].code == KEY_A)
						{
							cls ();
							printf ("---\n");
						}
						if (ev[0].code == KEY_B)
						{
							break;
						}
					}
				}
			}
			if (polldat[1].revents) // tsr
			{
				int color=0x0000ff;
				static int x=-1, y=-1, p=-1, state=0;
				int rd = read(tsr, ev, sizeof(struct input_event) * 64);
				for (i = 0; i < rd / sizeof(struct input_event); i++)
				{
					switch (ev[i].type)
					{
					case EV_SYN:
						if (x<0 || y<0 || p<0)
							break;
						if (p>0)
						{
							if (rlx != -1 && rly != -1)
							{
								// Existing
								line (rlx, rly, x, y, color);
								double dx=(rlx-x);
								double dy=(rly-y);
								double d = sqrt (dx*dx+dy*dy);
								avgdr = .98*avgdr + .02*d;
								printf ("r %4.1f %4.1f\n", d, avgdr);
								sprintf (buf, "%4d", (int)avgdr);
								drawtext (0,0, buf);

								int h=(int)d/HW;
								if (h<0) h=0;
								if (h>HN-1) h=HN-1;
								hr[h]++;
								int xx=hsize-hr[h];
								int yy=h*vsize/HN;
								line (xx,yy,xx,yy+vsize/HN-1, color);
							}
							rlx = x;
							rly = y;
							fullfatdot (rlx, rly, 2, color);
						}
						else
							rlx = rly = -1;
						break;
					case EV_ABS:
						switch (ev[i].code)
						{
						case ABS_X: x=ev[i].value; break;
						case ABS_Y: y=ev[i].value; break;
						case ABS_PRESSURE: p=ev[i].value; break;
						}
						break;
					case EV_KEY: state = ev[i].value;
					}
				}
			}
		}
	}
	ts_close (tsl);
	exit_drawtext ();
	exit_fb ();
}

