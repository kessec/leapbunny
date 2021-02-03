// Check For noise on an unmoving touch using input device

#include <stdint.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Stats.cpp"
#include <tslib.h>

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

	struct tsdev *tsl = ts_open(argv[1], 1);
	if (!tsl) {
		perror("ts_open");
		return 1;
	}

	if (ts_config(tsl)) {
		perror("ts_config");
		return 1;
	}
	fd = ts_fd (tsl);
	if (fd < 0)
	{
		perror("ts-input-noise");
		return 1;
	}
	if (argc>2)
		N=atoi(argv[2]);

	CStats sx, sy, sp;

	// Throw out samples until input events are all initd
	int spares = 5;
	for (i=0; i<N;)
	{
		struct ts_sample samp;
		int rd = ts_read(tsl, &samp, 1);
		if (rd && samp.x>0 && samp.y>0 && samp.pressure>0)
		{
			if (spares)
				spares--;
			else
			{
				sx.Add(samp.x);
				sy.Add(samp.y);
				sp.Add(samp.pressure);
				i++;
			}
		}
	}
	printf ("%.0f\n", sx.N());
	printf ("x=%4.0f/%4.0f/%4.0f %.2f\n", sx.Min(), sx.Mean(), sx.Max(), sx.Stdev());
	printf ("y=%4.0f/%4.0f/%4.0f %.2f\n", sy.Min(), sy.Mean(), sy.Max(), sy.Stdev());
	printf ("p=%4.0f/%4.0f/%4.0f %.2f\n", sp.Min(), sp.Mean(), sp.Max(), sp.Stdev());
}
