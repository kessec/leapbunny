#include <stdint.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

char *label[] = {"x1", "x2", "y1", "y2", "p1", "p2", "t1", "t2", "Pr", "Dy"};

#define N	10
#define N0	0

int main (int argc, char **argv)
{
	int fd, i;
	struct input_event ev[64];
	char name[256] = "Unknown";
	static int x[N];
	char buf[100];
	memset (buf, ' ', 100-1);
	buf[N*5+30+1] = 0;

	if (argc < 2) {
		printf("Usage: evtest /dev/input/eventX\n");
		printf("Where X = input device number\n");
		return 1;
	}

	if ((fd = open(argv[argc - 1], O_RDONLY)) < 0) {
		perror("myinput");
		return 1;
	}

	ioctl(fd, EVIOCGNAME(sizeof(name)), name);
	printf("Input device name: \"%s\"\n", name);

	for (i=0; i<N; i++)
		x[i] = -1;
	while (1) {
		int q=0;
		memset (buf, ' ', 30);
		int rd = read(fd, ev, sizeof(struct input_event) * 64);
		printf ("%2d: ", rd / sizeof(struct input_event));
		for (i = 0; i < rd / sizeof(struct input_event); i++)
		{
			char buf3[5];
			int j;
			switch (ev[i].type)
			{
			case EV_ABS: 
				j = ev[i].code-N0;
				if (j>=0 && j<N)
				{
					x[j] = ev[i].value;
					sprintf (buf3, "%4d", ev[i].value); 
					memcpy (buf+j*3, &label[j][0], 2);
					memcpy (buf+j*5+30, buf3, 4);
				}
				break;
			}
		}
		printf ("%s\n", buf);
	}
}
