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
	int fd, i, j, rd;
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
	fd = open (argv[1], O_RDONLY);
	if (fd < 0)
	{
		perror("ts-input-noise");
		return 1;
	}
	if (argc>2)
		N=atoi(argv[2]);

	CStats tx, ty, tp;
	CStats ix, iy, ip;

	// Throw out samples until input events are all initd
	int spares = 5;
	for (i=0; i<N;)
	{
		static int x=-1, y=-1, p=-1;
		struct ts_sample samp[64];
		struct input_event ev[64];

		// Input...
		rd = read(fd, &ev[0], sizeof(struct input_event) * 64);
		if (rd > 0)
			rd /= sizeof(struct input_event);
		for (j=0; j<rd; j++)
		{
			switch (ev[j].type)
			{
			case EV_SYN:
				if (x>0 && y>0 && p>0)
				{
					if (spares)
						spares--;
					else
					{
						ix.Add(x);
						iy.Add(y);
						ip.Add(p);
					}
				}
				break;
			case EV_ABS:
				switch (ev[j].code)
				{
				case ABS_X: x=ev[j].value; break;
				case ABS_Y: y=ev[j].value; break;
				case ABS_PRESSURE: p=ev[j].value; break;
				}
				break;
			}
		}
		// tslib
		rd = ts_read(tsl, &samp[0], 64);
		for (j=0; j<rd; j++)
		{
			if (!spares)
			{
				tx.Add(samp[j].x);
				ty.Add(samp[j].y);
				tp.Add(samp[j].pressure);
				i++;
			}
		}
	}
	printf ("Input: %.0f\n", ix.N());
	printf ("x=%4.0f/%4.0f/%4.0f %.2f\n", ix.Min(), ix.Mean(), ix.Max(), ix.Stdev());
	printf ("y=%4.0f/%4.0f/%4.0f %.2f\n", iy.Min(), iy.Mean(), iy.Max(), iy.Stdev());
	printf ("p=%4.0f/%4.0f/%4.0f %.2f\n", ip.Min(), ip.Mean(), ip.Max(), ip.Stdev());
	printf ("Tslib: %.0f\n", tx.N());
	printf ("x=%4.0f/%4.0f/%4.0f %.2f\n", tx.Min(), tx.Mean(), tx.Max(), tx.Stdev());
	printf ("y=%4.0f/%4.0f/%4.0f %.2f\n", ty.Min(), ty.Mean(), ty.Max(), ty.Stdev());
	printf ("p=%4.0f/%4.0f/%4.0f %.2f\n", tp.Min(), tp.Mean(), tp.Max(), tp.Stdev());
}
