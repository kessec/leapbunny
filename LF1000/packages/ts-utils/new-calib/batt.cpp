// Mister Brush: Demo of pressure painting

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
#include <malloc.h>
#include <unistd.h>

#include "fb.h"
#include "drawtext.h"

int main (int argc, char **argv)
{
	FILE *f = fopen ("/sys/devices/platform/lf1000-power/voltage", "r");
	if (!f)
	{
		perror ("/sys/devices/platform/lf1000-power/voltage");
		return 1;
	}
	int dt = 0;
	if (!init_fb () && !init_drawtext())
		dt=1;
	int i;
	for (i=0; i<10; i++)
	{
		char buf[100];
		int v;
		fseek (f, 0, 0);
		fscanf (f, "%d", &v);
		sprintf (buf, "%4d ", v);
		if (dt) drawtext (0, 0, buf);
		printf ("%d\n", v);
		sleep (1);
	}
	return 0;
}

#if 0

	// Clear screen
	cls ();

	// Keyboard
	int kb = open_input_device(KB_NAME);
	if (kb < 0)
	{
		perror("Can't find " KB_NAME);
		return 1;
	}
	// Input Event 
	if (find_input_device (TS_NAME, ts_dev_name) < 0)
	{
		perror ("Can't find " TS_NAME);
		return 1;
	}
	int ts = open(ts_dev_name, O_RDONLY);
	if (ts < 0)
	{
		perror("failed to open ts device");
		return 1;
	}
	struct pollfd polldat[1] = {kb, POLLIN };
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

	// The main event loop
	while (1) {
		static int tlx=-1, tly=-1, tlf=0;
		struct ts_sample samp[64];
		struct input_event ev[64];
		int i;
		
		// tslib have something for us?  If so, draw it
		int ret = ts_read(tsl, samp, 64);
		for (i=0; i<ret; i++)
		{
			// Pen down?  (but skip if user didn't want it)
			if (samp[i].pressure)
			{
				int fatness = (samp[i].pressure+9)/10;
				// fatdot (samp[i].x, samp[i].y, fatness, 0x0);
				if (tlx != -1 && tly != -1)
					nib ? fullroundline (tlx, tly, samp[i].x, samp[i].y,
							     fatness, tlf, 0x0)
					    : fatline (tlx, tly, samp[i].x, samp[i].y,
						       fatness, tlf, 0x0);
				else
					nib ? fullrounddot (samp[i].x, samp[i].y, fatness, 0x0)
						: fatdot (samp[i].x, samp[i].y, fatness, 0x0);
				
				tlx = samp[i].x;
				tly = samp[i].y;
				tlf = fatness;
			}
			else
			{
				tlx = tly = -1;
				tlf = 0;
			}
		}
		
		// input event system: poll keyboard[1] and ts[0]
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
						if (ev[0].code == KEY_A)
						{
							cls ();
						}
						else if (ev[0].code == KEY_B)
						{
							save ();
						}
						else if (ev[0].code == KEY_UP || ev[0].code == KEY_DOWN)
						{
							if (load (load_x))
								load_x = 0;
							else
								load_x++;
						}
						else if (ev[0].code == KEY_P)
						{
							nib = !nib;
						}
					}
				}
			}
		}
	}
}

#endif
