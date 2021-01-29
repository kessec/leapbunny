#include <stdint.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define RAW 1 // set to 1 to use RAW

char *label[] = {"x1", "x2", "y1", "y2", "p1", "p2", "t1", "t2", "Pr", "Dy"};

#define N	10
#define N0	0

#include <sys/types.h>
#include <dirent.h>
#include <sys/ioctl.h>

#include <stdlib.h>

int find_input_device(const char *input_name, char *dev) 
{
	struct dirent *dp;
	char name[32];
	DIR *dir;
	int fd, i;

	dir = opendir("/dev/input/");
	if (!dir)
		return -1;
	
	while ((dp = readdir(dir)) != NULL) {
		if (dp->d_name && !strncmp(dp->d_name, "event", 5)) {
			sprintf(dev, "/dev/input/%s", dp->d_name);
			fd = open(dev, O_RDONLY);
			if (fd == -1)
				continue;
			int i=ioctl(fd, EVIOCGNAME(32), name);
			close (fd);
			if (i < 0)
				continue;
			if (!strcmp(name, input_name)) {
				closedir(dir);
				return 0;
			}
		}
	}
	closedir(dir);
	return -1;
}

// Find an input device by name and open it and return an fd or -1 for failure
int open_input_device(const char *input_name)
{
	char dev[20];
	if (find_input_device (input_name, dev) < 0)
		return -1;
	return open(dev, O_RDONLY);
}

int get_sysfs (const char *sysfs)
{
	char buf[100];
	sprintf (buf, "/sys/devices/platform/lf1000-touchscreen/%s", sysfs);
	FILE *f;
	f = fopen (buf, "r");
	if (!f) { 
		perror ("set_sysfs read"); 
		printf("Failed to open %s for reading\n", buf);
		exit (1); 
	}
	int x;
	if (fscanf (f, "%d", &x) < 1) { 
		printf ("scanf failed on %s\n", buf); 
		exit (1); 
	}
	fclose (f);
	return x;
}

void set_sysfs (const char *sysfs, int value)
{
	char buf[100];
	sprintf (buf, "/sys/devices/platform/lf1000-touchscreen/%s", sysfs);
	FILE *f;
	f = fopen (buf, "w");
	if (!f) { 
		perror ("set_sysfs write"); 
		printf("Failed to open %s for writing\n", buf);
		exit (1); 
	}
	fprintf (f, "%d\n", value);
	fclose (f);
}

int main (int argc, char **argv)
{
	int fd, i;
	struct input_event ev[64];
	char name[256] = "Unknown";
	static int x[N];
	char buf[100];
	memset (buf, ' ', 100-1);
	buf[N*5+30+1] = 0;

	set_sysfs ("sample_rate_in_hz", 100);
#if RAW	
	set_sysfs ("report_events", 5);
	fd = open_input_device ("LF1000 touchscreen raw");
#else
	fd = open_input_device ("LF1000 touchscreen interface");
#endif
	
	if (fd < 0)
	{
		perror("can't find input device");
		return 1;
	}

	for (i=0; i<N; i++)
		x[i] = -1;
#if !RAW
	x[2] = 1;
#endif
	while (1) {
		int rd = read(fd, ev, sizeof(struct input_event) * 64);
		for (i = 0; i < rd / sizeof(struct input_event); i++)
		{
			char buf3[5];
			int j;
			switch (ev[i].type)
			{
			case EV_SYN: 
				printf ("%4d %4d %4d (%4d, %4d) %4d\n",
#if RAW
					(x[0]+x[1]+1)/2,
					(x[2]+x[3]+1)/2,
					(x[6]+x[7]+1)/2,
					x[6],x[7],
					x[8]);
#else
					x[0], x[1], x[2], x[3]);
#endif

				break;
			case EV_ABS: 
#if RAW
				j = ev[i].code-N0;
#else
				j = ev[i].code-N0;
				if (j>3) j=3;
#endif
				if (j>=0 && j<N)
					x[j] = ev[i].value;
				break;
			}
		}
	}
}
