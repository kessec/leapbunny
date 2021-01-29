// Check For noise on an unmoving touch using input device

#include <stdint.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Stats.cpp"

#define UNKNOWN	-1000

int main (int argc, char **argv)
{
	int fd, i;
	struct input_event ev;
	int N = 100;

	if (argc<2)
	{
		printf ("usage\nts-input-noise /dev/input/event3 [N]\n");
		return 1;
	}
	fd = open (argv[1], O_RDONLY);
	if (fd < 0)
	{
		perror("ts-input-noise");
		return 1;
	}
	if (argc>2)
		N=atoi(argv[2]);

	CStats sx, sy, sp;


	for (i=0; i<N;)
	{
		static int x=UNKNOWN, y=UNKNOWN, p=UNKNOWN, state=UNKNOWN;
		int rd = read(fd, &ev, sizeof(struct input_event));
		if (rd < (int) sizeof(struct input_event)) {
			perror("\nmyinput: error reading");
			return 1;
		}
		switch (ev.type)
		{
		case EV_SYN: 
			if (x!=UNKNOWN && y!=UNKNOWN && p!=UNKNOWN)
			{
				sx.Add(x);
				sy.Add(y);
				sp.Add(p);
				i++;
			}
			break;
		case EV_ABS: 
			switch (ev.code)
			{
			case ABS_X: x=ev.value; break;
			case ABS_Y: y=ev.value; break;
			default:
			case ABS_PRESSURE: p=ev.value; break;
			}
			break;
		case EV_KEY: 
			state = ev.value;
		}
	}
	printf ("%.0f\n", sx.N());
	printf ("x=%4.0f/%4.0f/%4.0f %.2f\n", sx.Min(), sx.Mean(), sx.Max(), sx.Stdev());
	printf ("y=%4.0f/%4.0f/%4.0f %.2f\n", sy.Min(), sy.Mean(), sy.Max(), sy.Stdev());
	printf ("p=%4.0f/%4.0f/%4.0f %.2f\n", sp.Min(), sp.Mean(), sp.Max(), sp.Stdev());
}
