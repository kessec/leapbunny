// Check For noise on an unmoving touch using input device

#include <stdint.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

int main (int argc, char **argv)
{
	int fd, i;
	struct timeval t0, t1, t2;
	struct input_event ev;
	long t;

	fd = open (argv[1], O_RDONLY);
	if (fd < 0)
	{
		perror("ts-input-bandwidth");
		return 1;
	}

	gettimeofday (&t0, NULL);
	while (1)
	{
		static int x=0, y=0, p=0, k=0, s=0;
		int rd = read(fd, &ev, sizeof(struct input_event));
		if (rd < (int) sizeof(struct input_event)) {
			perror("\nmyinput: error reading");
			return 1;
		}
		switch (ev.type)
		{
		case EV_SYN: s++; break;
		case EV_ABS: 
			switch (ev.code)
			{
			case ABS_X: x++; break;
			case ABS_Y: y++; break;
			case ABS_PRESSURE: p++; break;
			}
			break;
		case EV_KEY: k++; break;
		}
		gettimeofday (&t1, NULL);
		if (t1.tv_sec == t2.tv_sec)
			continue;
		double f=t1.tv_sec-t0.tv_sec+(t1.tv_usec-t0.tv_usec)*1e-6;
		// printf ("x=%d y=%d p=%d sync=%d key=%d", x, y, p, s, k);
		printf ("x=%.1f y=%.1f p=%.1f sync=%.1f key=%.1f\n", 
			x/f, y/f, p/f, s/f, k/f);
		// x=0; y=0; p=0; s=0; k=0;
		gettimeofday (&t2, NULL);
	}
}
