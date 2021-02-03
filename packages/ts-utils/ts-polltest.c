


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

#include <linux/lf1000/mlc_ioctl.h>

#include <tslib.h>

#define LAYER	"/dev/layer0"
#define MLC	"/dev/mlc"
#define TS_NAME	"LF1000 touchscreen interface"

char ts_dev_name[20];

// Borrow some code to look up event devices
#include "open-input.c"

int main (int argc, char **argv)
{
	// Input Event 
	if (find_input_device (TS_NAME, ts_dev_name) < 0)
	{
		perror ("Can't find " TS_NAME);
		return 1;
	}
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
	int ts = open(ts_dev_name, O_RDONLY);
	if (ts < 0)
	{
		perror("failed to open ts device");
		return 1;
	}
	int tsp = ts_fd (tsl);
	// Poll both [0]=the input device and [1]=tslib's fd
	struct pollfd polldat[2] = {{ ts, POLLIN }, { tsp, POLLIN }};

	// The main event loop
	while (1) {
		static int x=-1, y=-1, p=-1, state=-1;
		struct ts_sample samp[64];
		struct input_event ev[64];
		int i;
		
		// Poll first
		if (poll (polldat, 2, 0) > 0)
		{
			if (polldat[0].revents) // input fd has activity
			{
				// Must be TS input...
				int rd = read(ts, ev, sizeof(struct input_event) * 64);
				if (rd < sizeof(struct input_event))
					printf ("\t\t\tinput NO SAMPLES READY\n");
				for (i = 0; i < rd / sizeof(struct input_event); i++)
				{
					switch (ev[i].type)
					{
					case EV_SYN:
						printf ("\t\t\tinput %4d,%4d,%4d\n", x, y, p);
						break;
					case EV_ABS:
						switch (ev[i].code)
						{
						case ABS_X: x=ev[i].value; break;
						case ABS_Y: y=ev[i].value; break;
						case ABS_PRESSURE: p=ev[i].value; break;
						}
						break;
					case EV_KEY:
						printf ("\t\t\tinput                %d\n", ev[i].value);
						break;
					}
				}
			}
			if (polldat[1].revents) // tslib's fd has activity
			{
				// tslib have something for us?  If so, draw it
				int ret = ts_read(tsl, samp, 64);
				if (ret==0)
					printf ("tslib NO SAMPLES READY\n");
				for (i=0; i<ret; i++)
				{
					// Pen down?  (but skip if user didn't want it)
					printf ("tslib %4d,%4d,%4d\n", samp[i].x, samp[i].y, samp[i].pressure);
				}
			}
		}
	}
}

