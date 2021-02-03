#include <stdio.h>
#include <linux/input.h>
#include <tslib.h>
#include <fcntl.h>
#include <sys/time.h>
main ()
{
	int i;
	struct ts_sample ts;
	struct input_event ev;
	char which[] = "/dev/input/event3";

	struct tsdev *tsl = ts_open(which, 0);
	if (!tsl) {
		perror("ts_open");
		return 1;
	}
	if (ts_config(tsl)) {
		perror("ts_config");
		return 1;
	}

	/* int ff = open (which, O_RDONLY); */

	int fd = open(which, O_WRONLY | O_SYNC); // DSYNC);
	if (fd<0) { perror ("open which for write"); return 1; }

	gettimeofday (&ev.time, NULL);
	ev.type=EV_ABS; ev.code=ABS_X; ev.value=999;
	write (fd, &ev, sizeof(ev));
	gettimeofday (&ev.time, NULL);
	ev.type=EV_SYN; ev.code=1; ev.value=0;
	write (fd, &ev, sizeof(ev));
	write (fd, &ev, sizeof(ev));
	int ret0 = ts_read(tsl, &ts, 1);
	printf ("ret0=%d\n", ret0);

	int dy = 10;
	int dx = 100;
	int x0=10, x, y=10;
	for (i=0; i<6; i++)
	{
		write (fd, &ev, sizeof(ev));
		if (i==3) 
			x = x0;
		else
			x = x0+dx;
		ev.type=EV_ABS; ev.code=ABS_X; ev.value=x;
		int j = write (fd, &ev, sizeof(ev));
		ev.type=EV_ABS; ev.code=ABS_Y; ev.value=y;
		write (fd, &ev, sizeof(ev));
		ev.type=EV_ABS; ev.code=ABS_PRESSURE; ev.value=10;
		write (fd, &ev, sizeof(ev));
		ev.type=EV_SYN; ev.code=1; ev.value=i;
		write (fd, &ev, sizeof(ev));
		y += dy;
	}
	ev.type=EV_ABS; ev.code=ABS_PRESSURE; ev.value=0;
	write (fd, &ev, sizeof(ev));
	ev.type=EV_SYN; ev.code=1; ev.value=i;
	write (fd, &ev, sizeof(ev));
	close (fd);

	while (1)
	{
		int ret = ts_read(tsl, &ts, 1);
		if (ret < 1)
			return 0;
		printf ("tslib %d,%d;%d\n", ts.x, ts.y, ts.pressure);
	}

	/* while (1) */
	/* { */
	/* 	int ret = read (ff, &ev, sizeof(ev)); */
	/* 	if (ret < 1) */
	/* 		return 0; */
	/* 	printf ("got one...\n"); */
	/* } */
}
