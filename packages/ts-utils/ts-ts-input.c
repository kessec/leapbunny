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

#include "open-input.c"

char dev[20];

int main (int argc, char **argv)
{
	// tslib
	if (find_input_device("LF1000 touchscreen interface", dev) < 0) {
		perror ("Can't find touchscreen event device in /dev/input");
		return 1;
	}
	struct tsdev *tsl = ts_open(dev, 1);
	if (!tsl) {
		perror("ts_open");
		return 1;
	}
	if (ts_config(tsl)) {
		perror("ts_config");
		return 1;
	}
	// Input
	int ts = open(dev, O_RDONLY);
	if (ts < 0)
	{
		perror("failed to open touchscreen device");
		return 1;
	}
	struct pollfd polldat = { ts, POLLIN };

	while (1) {
		static int x=-1, y=-1, p=-1, state=-1;
		struct ts_sample samp[64];
		struct input_event ev[64];
		int i;
		
		// input event system
		if (poll (&polldat, 1, 0) > 0)
		{
			int rd = read(ts, ev, sizeof(struct input_event) * 64);
			for (i = 0; i < rd / sizeof(struct input_event); i++)
			{
				switch (ev[i].type)
				{
				case EV_SYN: 
					printf ("\t\t\tinput %d,%d;%d (%d)\n", x, y, p, state); break;
				case EV_ABS:
					switch (ev[i].code)
					{
					case ABS_X: x=ev[i].value; break;
					case ABS_Y: y=ev[i].value; break;
					case ABS_PRESSURE: p=ev[i].value; break;
					}
					break;
				case EV_KEY: state = ev[i].value; 
					printf ("\t\t\tinput state (%d)\n", state); break;
				}
			}
		}
		// tslib
		int ret = ts_read(tsl, samp, 64);
		for (i=0; i<ret; i++)
		{
			printf ("tslib %d,%d;%d\n", samp[i].x, samp[i].y, samp[i].pressure);
		}
	}
}

