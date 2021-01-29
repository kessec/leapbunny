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
#include <sys/ioctl.h>
#include <malloc.h>
#include <unistd.h>
#include <poll.h>
#include <stdlib.h>

#include <tslib.h>

#define TS_NAME	"LF1000 touchscreen interface"
#define KB_NAME	"LF1000 Keyboard"

char ts_dev_name[20];

#include "fb.h"
#include "drawtext.h"
#include "OpenInput.h"

void save ()
{
	// Find next possible file name
	char name[100];
	int x=0;
	while (x<10000)
	{
		struct stat st;
		sprintf (name, "/LF/Bulk/mb_%04d.argb", x);
		if(stat(name, &st) != 0) /* file does not exist */
			break;
		// Try again
		x++;
	}
	int fd = open (name, O_WRONLY | O_CREAT, 0666);
	if (fd < 0)
	{
		perror ("Can't open for saving");
		return;
	}
	write (fd, dc, hsize*vsize*hstride);
	close (fd);
	drawtext (hsize/2-8*8/2, vsize/2, "Saving...");
	printf ("Saved %s: ", name);
	printf ("Use 'convert -size %dx%d -depth 8 -alpha off rgba:mb_%04d.argb mb_%04d.png'\n", hsize, vsize, x, x);
	sleep (1);
	cls ();
}

int load (int x)
{
	// Find next possible file name
	char name[100];
	struct stat st;
	sprintf (name, "/LF/Bulk/mb_%04d.argb", x);
	if(stat(name, &st) != 0) /* file does not exist */
		return -1;
	int fd = open (name, O_RDONLY);
	if (fd < 0)
	{
		perror ("Can't open for loading");
		return -1;
	}
	read (fd, dc, hsize*vsize*hstride);
	close (fd);
	return 0;
}


int main (int argc, char **argv)
{
	int load_x = 0;
	int nib = 1;
	int config = 1;
	printf ("%s: Mr Brush 2.1\n", argv[0]);
	printf ("Press A to clear screen, B or ESC to Save, Vol Down to exit\n");
	printf ("Press Up/Down to Load\n");
	printf ("Press Left/Right to change settings\n");
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


	// The main event loop
	while (1) {
		static int tlx=-1, tly=-1, tlf=0;
		struct ts_sample samp[64];
		struct input_event ev[64];
		int fatness=1;
		int i;
		
		// tslib have something for us?  If so, draw it
		int ret = ts_read(tsl, samp, 64);
		for (i=0; i<ret; i++)
		{
			long color=0;
			// Pen down?  (but skip if user didn't want it)
			int fatness = (samp[i].pressure+9)/10;
			if (!fatness)
				fatness = tlf;
			// fatdot (samp[i].x, samp[i].y, fatness, 0x0);
			if (tlx != -1 && tly != -1)
				nib ? fullroundline (tlx, tly, samp[i].x, samp[i].y,
						     tlf, fatness, 0x0)
					: fatline (tlx, tly, samp[i].x, samp[i].y,
						   tlf, fatness, 0x0);
			else
				nib ? fullrounddot (samp[i].x, samp[i].y, fatness, 0x0)
					: fatdot (samp[i].x, samp[i].y, fatness, 0x0);

			tlx = samp[i].x;
			tly = samp[i].y;
			tlf = fatness;
			if (!samp[i].pressure)
			{
				tlx = tly = -1;
				tlf = 0;
			}
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
						if (ev[0].code == KEY_A)
						{
							cls ();
						}
						else if (ev[0].code == KEY_B || ev[0].code == KEY_ESC)
						{
							save ();
						}
						else if (ev[0].code == KEY_VOLUMEDOWN)
						{
							exit (0);
						}
						else if (ev[0].code == KEY_UP || ev[0].code == KEY_DOWN)
						{
							cls ();
							if (load (load_x))
								load_x = 0;
							else
								load_x++;
						}
						else if (ev[0].code == KEY_LEFT || ev[0].code == KEY_RIGHT)
						{
							char buf[30];
							config = !config;
							sprintf (buf, "/flags/set-ts%s.sh", config ? "2" : "");
							system (buf);
							drawtext (0, 0, config ? "2" : "1");
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

