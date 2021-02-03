#include <stdio.h>
#include <linux/input.h>
#include <tslib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
main ()
{
	int i,j,k, fd;
	struct ts_sample ts;
	struct input_event ev;
	char which[] = "/sys/devices/platform/lf1000-touchscreen/us";
	char buf[100];

	int dy = 100;
	int dx = 200;
	int pos;
#define N 6	
	for (j=0; j<4; j++)
	{
		int LUT[] =  { 0, 10, 20, 40 };
		int LUT2[] = { 0, 10, 30, 60 };
		for (k=0; k<N; k++)
		{
			dx = LUT[j];
			int p=10;
			int x, x0 = LUT2[j];
			int y = 10 + k*8*dy;
			for (i=0; i<6; i++)
			{
				if (i==k) 
					x = x0+dx;
				else
					x = x0;
				sprintf (buf, "%u %u %u\n", x, y, p);
				printf (buf, "Sending: %u %u %u\n", x, y, p);
				fd = open(which, O_WRONLY); // DSYNC);
				if (fd<0) { perror ("open which for write"); return 1; }
				write (fd, buf, strlen(buf));
				close (fd);
				y += dy;
			}
			sprintf (buf, "%u %u %u\n", x, y-dy, 0);
			fd = open(which, O_WRONLY); // DSYNC);
			if (fd<0) { perror ("open which for write"); return 1; }
			write (fd, buf, strlen(buf));
			sprintf (buf, "%u %u %u\n", 0, 0, 0);
			write (fd, buf, strlen(buf));
			close (fd);
			sleep (1);
		}
	}
}
