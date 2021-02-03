#include <stdint.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define UNKNOWN	-1000

int main (int argc, char **argv)
{
	int fd, i;
	struct input_event ev[64];
	char name[256] = "Unknown";
	int abs[5];
	char bits[(KEY_MAX+7)/8];
	
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

	// Find 3rd axis: Z (accel) or P (touch)
	int p_is_on=ABS_PRESSURE;
	ioctl (fd, EVIOCGBIT(EV_ABS, KEY_MAX), bits);
	if (bits[ABS_Z/8] & (1<<(ABS_Z%8)))
		p_is_on=ABS_Z;
	printf ("p is on %d\n", p_is_on);

	// prime the pump
	int x=UNKNOWN, y=UNKNOWN, p=UNKNOWN, state=UNKNOWN;
	ioctl (fd, EVIOCGABS(ABS_X), abs); x=abs[0];
	ioctl (fd, EVIOCGABS(ABS_Y), abs); y=abs[0];
	ioctl (fd, EVIOCGABS(p_is_on), abs); p=abs[0];
	printf ("initial x=%d y=%d p=%d\n", x, y, p);
	while (1) {
		char buf2[20];
		strcpy (buf2, "                   ");
		int q=0;

		int rd = read(fd, ev, sizeof(struct input_event) * 64);

		if (rd < (int) sizeof(struct input_event)) {
			perror("\nmyinput: error reading");
			return 1;
		}

		printf ("%d: ", rd / sizeof(struct input_event));
		for (i = 0; i < rd / sizeof(struct input_event); i++)
		{
			char *n="?";
			char buf3[5];
			switch (ev[i].type)
			{
			case EV_SYN: 
				buf2[15] = 'S'; 
				break;
			case EV_ABS: 
				if (ev[i].code==8) break; // toss ticks from acclr
				else if (ev[i].code==0) x=ev[i].value; 
				else if (ev[i].code==1) y=ev[i].value; 
				else p=ev[i].value;
				sprintf (buf3, "%4d", ev[i].value); 
				memcpy (buf2+(ev[i].code==0?0:
					      ev[i].code==1?5:10), buf3, 4);
				break;
			case EV_KEY: 
				state = ev[i].value;
				break;
			}
		}
		if (state)
			printf ("%-17s D %4d %4d %4d\n", buf2, x, y, p);
		else
			printf ("%-17s U\n", buf2);

	}
}
