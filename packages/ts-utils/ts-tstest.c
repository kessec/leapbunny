#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>
#include <tslib.h>
#include <fcntl.h>

#include "open-input.c"

int main ()
{
	struct tsdev *ts;
	char *tsdevice=NULL;
	int version;
	u_int32_t bit;
	u_int64_t absbit;
	char dev[20];
	if (find_input_device("LF1000 touchscreen interface", dev) < 0) {
		perror ("Can't find touchscreen event device in /dev/input");
		exit(1);
	}
	int fd = open_input_device("LF1000 touchscreen interface");
	int a = ioctl(fd, EVIOCGVERSION, &version);
	int b = ioctl(fd, EVIOCGBIT(0, sizeof(bit) * 8), &bit);
	int c = ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbit) * 8), &absbit);
	printf ("fd=%d a=%d b=%d c=%d version=%x bit=%x absbit=%x\n",
		fd, a, b, c, version, bit, (int) absbit);

	ts = ts_open(dev, 0);
	if (!ts) {
		perror("ts_open");
		exit(1);
	}

	if (ts_config(ts)) {
		perror("ts_config");
		exit(1);
	}

	while (1) {
		struct ts_sample samp;
		int ret;

		ret = ts_read(ts, &samp, 1);

		if (ret < 0) {
			perror("ts_read");
			exit(1);
		}

		if (ret != 1)
			continue;

		printf("%ld.%06ld: %6d %6d %6d\n", samp.tv.tv_sec, samp.tv.tv_usec, samp.x, samp.y, samp.pressure);

	}
}
