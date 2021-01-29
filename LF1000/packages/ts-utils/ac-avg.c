#include <stdint.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

double xbias=2.20, ybias=-2.27, zbias=1.15;

int main (int argc, char **argv)
{
	int fd, i, j;
	struct input_event ev[64];
	char name[256] = "Unknown";
	int abs[5];
	char bits[(KEY_MAX+7)/8];
	int sx=0, sy=0, sz=0;

	if (argc < 2) {
		printf("Usage: ac-avg /dev/input/eventX\n");
		printf("Where X = input device number\n");
		return 1;
	}

	if ((fd = open(argv[argc - 1], O_RDONLY)) < 0) {
		perror("myinput");
		return 1;
	}

	ioctl(fd, EVIOCGNAME(sizeof(name)), name);
	printf("Input device name: \"%s\"\n", name);

	// prime the pump
	int x, y, z, r;
	r=ioctl (fd, EVIOCGABS(ABS_X), abs); x=abs[0];
	if (r) { perror ("ioclt ABS_X failed\n"); return 1; }
	r=ioctl (fd, EVIOCGABS(ABS_Y), abs); y=abs[0];
	if (r) { perror ("ioclt ABS_Y failed\n"); return 1; }
	r=ioctl (fd, EVIOCGABS(ABS_Z), abs); z=abs[0];
	if (r) { perror ("ioclt ABS_Z failed\n"); return 1; }
	printf ("x=%d y=%d z=%d\n", x, y, z);
	int N=100;
	for (j=0; ;)
	{		
		int rd = read(fd, ev, sizeof(struct input_event) * 64);
		if (rd < (int) sizeof(struct input_event)) {
			perror("\nmyinput: error reading");
			return 1;
		}
		for (i = 0; i < rd / sizeof(struct input_event); i++)
		{
			switch (ev[i].type)
			{
			case EV_SYN: 
				sx+=x; sy+=y; sz+=z;
				j++;
				if (j>=10)
				{
					double gx=(double)sx/j;
					double gy=(double)sy/j;
					double gz=(double)sz/j;
					double d = sqrt(gx*gx+gy*gy+gz*gz);
					double l=14.5;
					double bx=gx*(l-d)/d;
					double by=gy*(l-d)/d;
					double bz=gy*(l-d)/d;
					printf ("%6.2f %6.2f %6.2f d=%6.2f ",
						gx, gy, gz, d);
					printf ("%6.2f %6.2f %6.2f\n",
						bx, by, bz);
					j=0;
					sx = sy = sz = 0;
				}
				break;
			case EV_ABS: 
				if (ev[i].code==8) break; // toss ticks
				else if (ev[i].code==0) x=ev[i].value; 
				else if (ev[i].code==1) y=ev[i].value; 
				else z=ev[i].value;
				break;
			}
		}
	}
}
