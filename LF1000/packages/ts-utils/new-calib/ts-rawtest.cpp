// Taken from Mister Brush

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
#include <stdlib.h>

// #include <tslib.h>

#define TSR_NAME	"LF1000 touchscreen raw"
#define KB_NAME		"LF1000 Keyboard"

char ts_dev_name[20];

#include "fb.h"
#include "drawtext.h"
#include "OpenInput.h"

#define WHITE 0xffffff
#define BLACK 0x000000
#define RED   0xff0000

#define VERSION "1.3"

#define H_OFF -1
#define H_MIN 0
#define H_MAX 10
#define H_N (H_MAX-H_MIN+1)
#define HQ_N 100

int hq[HQ_N], hqi=0, hx, hy;

void h_init (int x, int y)
{
	int i;
	for (i=0; i<HQ_N; i++)
		hq[i] = H_OFF;
	hx = x;
	hy = y;
}

void h_add (int x)
{
	hq[hqi++] = x;
	hqi %= HQ_N;
}

void h_plot ()
{
	int i, x=hx, y=hy;
	
	int h[H_N];
	memset (h, 0, sizeof (h));
	for (i=0; i<HQ_N; i++)
		if (hq[i] >= H_MIN && hq[i] <= H_MAX)
			h[hq[i]-H_MIN]++;
	for (i=H_MIN; i<=H_MAX; i++)
	{
		char buf[10];
		sprintf (buf, "%2d: %2d", i, h[i-H_MIN]);
		drawtext (hx, y, buf);
		y += 12;
	}
}	


#define GX	(hires_grid ? 15 : 9)
#define GY	(hires_grid ?  9 : 5)

#define MAXGX	15
#define MAXGY	9

int hires_grid = 0;
int gx[MAXGX], gy[MAXGY], gstate;
double g[MAXGX*MAXGY];

void g_init ()
{
	int i;
	for (i=0; i<GX; i++)
		gx[i] = 10 + (int)(1.0*i/(GX-1)*(hsize-20));
	for (i=0; i<GY; i++)
		gy[i] = 10 + (int)(1.0*i/(GY-1)*(vsize-20));
	gstate = 0;
}

void g_plot (int color)
{
	int i, j;
	i = GX - 1 - gstate / GY;
	j = gstate % GY;
	fullfatdot (gx[i], gy[j], 5, color);
}

int g_add ()
{
	int i,j;

	g_plot (WHITE);
	gstate++;
	if (gstate==GX*GY)
	{
		int gs = 0;
		for (i=0; i<GX; i++)
		{
			for (j=0; j<GY; j++)
				printf ("%6.1f ", g[gs++]);
			printf ("\n");
		}
		return 1;
	}
	g_plot (BLACK);
	return 0;
}

int do_histo = 0;
int do_avg = 0;
int do_grid = 0;
int do_pressure = 0;

void help (char **argv)
{
	printf ("Usage:\n   %s [-l] [-p] [-d] [-m] [-h]\n", argv[0]);
	printf ("-H: Histogram of tnt1-tnt2\n");
	printf ("-a: averaging\n");
	printf ("-g: collect a 9x5 grid of data.  Press Home to advance target \n");
	printf ("-G: collect a 15x9 grid of data\n");
	printf ("-p: use pressure instead of TNT\n");
	printf ("-h: help\n");
}

int main (int argc, char **argv)
{
	char buf[100];
	int tsd[20], x, y, t, p, fancy_tnt, color, i, this_val;
	int tlx=-1, tly=-1;
	int ret=0;

	for (i=1; i<argc; i++)
	{
		if (!strcmp (argv[i], "-h"))
			help (argv), exit (0);
		if (!strcmp (argv[i], "-H"))
			do_histo = 1;
		if (!strcmp (argv[i], "-a"))
			do_avg = 1;
		if (!strcmp (argv[i], "-g"))
			do_avg = 1, do_grid = 1;
		if (!strcmp (argv[i], "-G"))
			do_avg = 1, do_grid = 1, hires_grid = 1;
		if (!strcmp (argv[i], "-p"))
			do_pressure = 1;
	}


	printf ("%s Version %s -- Press Esc to exit\n", argv[0], VERSION);

	if (init_fb ())
		return 1;
	if (init_drawtext())
		return 1;
	memset (tsd, 0xff, sizeof(tsd));

	// Clear screen
	cls ();

#define SYSFS "/sys/devices/platform/lf1000-touchscreen/"

	system ("echo 5 > " SYSFS "report_events");
	system ("echo 50 > " SYSFS "sample_rate_in_hz");

	FILE *FIN = fopen (SYSFS "pointercal", "r");
	if (!FIN)
	{
		perror("Can't open " SYSFS "pointercal");
		return 1;
	}
	int a[7];
	fscanf (FIN, "%d %d %d %d %d %d %d", 
				 &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6]);
	fclose (FIN);

	FIN = fopen (SYSFS "tnt_plane", "r");
	if (!FIN)
	{
		perror("Can't open " SYSFS "tnt_plane");
		return 1;
	}
	int tnt_plane[3];
	fscanf (FIN, "%d %d %d", &tnt_plane[0], &tnt_plane[1], &tnt_plane[2]);
	fclose (FIN);

	FIN = fopen (SYSFS "max_tnt_down", "r");
	if (!FIN)
	{
		perror("Can't open " SYSFS "max_tnt_down");
		return 1;
	}
	int max_tnt_down;
	fscanf (FIN, "%d", &max_tnt_down);
	fclose (FIN);

	// Keyboard
	int kb = open_input_device(KB_NAME);
	if (kb < 0)
	{
		perror("Can't find " KB_NAME);
		return 1;
	}
	// Touchscreen
	int ts = open_input_device(TSR_NAME);
	if (ts < 0)
	{
		perror("Can't find " TSR_NAME);
		return 1;
	}
	struct pollfd polldat[2] = {{kb, POLLIN}, {ts, POLLIN }};

	if (do_histo)
		h_init (5, 85);

	if (do_grid)
		g_init (), g_plot (BLACK);


	double avg = 0;
	int tails=0;

	// The main event loop
	while (1) {
		struct input_event ev[64];
		int i, wx, wy;

		// poll input event system
		if (poll (polldat, 2, 0) > 0)
		{
			if (polldat[1].revents) // Touchscreen
			{
				int rd = read(ts, ev, sizeof(struct input_event));
				if (rd == sizeof (struct input_event))
				{
					switch (ev[0].type)
					{
					case EV_ABS:
						tsd[ev[0].code] = ev[0].value;
						break;
					case EV_SYN:
						// Do pointercal transfrom from raw (wx, wy) to screen (x,y)
						wx = (tsd[0]+tsd[1])/2;
						wy = (tsd[2]+tsd[3])/2;
						x = (a[0]*wx + a[1]*wy + a[2])/a[6];
						y = (a[3]*wx + a[4]*wy + a[5])/a[6];

						// Compute TNT and up/down status
						t = (tsd[6]+tsd[7])/2;
						fancy_tnt = max_tnt_down + 
									(tnt_plane[0]*wx + tnt_plane[1]*wy 
									 + tnt_plane[2] + 32768)/65536;
						color = t < fancy_tnt ? BLACK : RED;

						if (tlx >= 0 && tlx < hsize && tlx != x)
							line (tlx, 0, tlx, vsize-1, WHITE), 
							line (tlx-1, 0, tlx-1, vsize-1, WHITE), 
							line (tlx+1, 0, tlx+1, vsize-1, WHITE);
						if (tly >=0 && tly < vsize && tly != y)
							line (0, tly, hsize-1, tly, WHITE), 
							line (0, tly-1, hsize-1, tly-1, WHITE), 
							line (0, tly+1, hsize-1, tly+1, WHITE);
						if (x >= 0 && x < hsize)
							line (x, 0, x, vsize-1, color), 
							line (x-1, 0, x-1, vsize-1, color), 
							line (x+1, 0, x+1, vsize-1, color);
						if (y >=0 && y < vsize)
							line (0, y, hsize-1, y, color), 
							line (0, y-1, hsize-1, y-1, color), 
							line (0, y+1, hsize-1, y+1, color);
						if (do_grid)
							g_plot (BLACK);
						tlx = x;
						tly = y;
						t = (tsd[6]+tsd[7])/2;
						int a=abs(tsd[6]-tsd[7]);
						if (t<fancy_tnt+20 && do_histo)
							h_add(a);
						if (do_pressure) {
							p = tsd[8];	
							this_val = p;
						}
						else {
							this_val = t;
						}
						if (do_avg)
						{
							avg = .9*avg + .1*this_val;
							if (do_pressure) {
								sprintf (buf, "P: %6.1f", avg);
							}
							else {
								sprintf (buf, "%6.1f (%2d) ", avg, fancy_tnt);
							}
						}
						else if (do_pressure) {
							sprintf (buf, "P: %4d", p);
						}
						else {
							sprintf (buf, "%4d (%2d)", t, fancy_tnt);
						}
						drawtext (5, 70, buf);
						if (do_histo)
							h_plot ();
					}
				}
			}
			if (polldat[0].revents) // Keyboard
			{
				int rd = read(kb, ev, sizeof(struct input_event));
				if (rd == sizeof (struct input_event))
				{
					if (ev[0].type == EV_KEY && 
					    ev[0].value==1)
					{
						if (ev[0].code == KEY_ESC)
						{
							if (!do_grid)
								break;
							else {
								g[gstate] = avg;
								if (g_add())
									break;
							}
						}
						if (ev[0].code == KEY_UP)
						{
							tails = !tails;
							if (tails)
								system ("echo 1 > " SYSFS "tails");
							else
								system ("echo 0 > " SYSFS "tails");
						}
					}
				}
			}
		}
	}
	system ("echo 1 > " SYSFS "report_events");
	system ("echo 50 > " SYSFS "sample_rate_in_hz");
	cls ();

	return 0;
}

