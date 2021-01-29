#include <stdint.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main (int argc, char **argv)
{
	int fd, i, fuzzx,fuzzy,fuzzp;
	struct input_absinfo abs;
	int set=0;

	if (argc<3 || (argc>3 && argc<6))
	{
		printf ("Usage: ts-setfuzz /dev/input/event3 set/get xfuzz yfuzz pressurefuzz\n");
		return 1;
	}
	fd = open (argv[1], O_RDONLY);
	if (fd < 0)
	{
		perror("ts-setfuzz");
		return 1;
	}
	if (tolower(argv[2][0]) == 's')
		set=1;
	fuzzx=fuzzy=fuzzp=0;
	if (argc>3)
	{
		fuzzx=atoi(argv[3]);
		fuzzy=atoi(argv[4]);
		fuzzp=atoi(argv[5]);
	}

	ioctl(fd, EVIOCGABS(ABS_X), &abs);
	if (set)
	{
		abs.fuzz=fuzzx;
		ioctl(fd, EVIOCSABS(ABS_X), &abs);
	}
	printf ("x min=%d max=%d fuzz=%d flat=%d\n", abs.minimum, abs.maximum,
		abs.fuzz, abs.flat);

	ioctl(fd, EVIOCGABS(ABS_Y), &abs);
	if (set)
	{
		abs.fuzz=fuzzy;
		ioctl(fd, EVIOCSABS(ABS_Y), &abs);
	}
	printf ("y min=%d max=%d fuzz=%d flat=%d\n", abs.minimum, abs.maximum,
		abs.fuzz, abs.flat);

	ioctl(fd, EVIOCGABS(ABS_PRESSURE), &abs);
	if (set)
	{
		abs.fuzz=fuzzp;
		ioctl(fd, EVIOCSABS(ABS_PRESSURE), &abs);
	}
	printf ("p min=%d max=%d fuzz=%d flat=%d\n", abs.minimum, abs.maximum,
		abs.fuzz, abs.flat);
}
