// Accelerometer Show

#define _FILE_OFFSET_BITS	64

#include <stdint.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <unistd.h>
#include <poll.h>
#include <math.h>

#define ACC_NAME	"LF1000 Accelerometer"
#define KB_NAME	"LF1000 Keyboard"

#include "fb.h"
#include "OpenInput.h"
#include "MaxMin.h"
#include "Histo.h"
#include "Stats.h"
#include "drawtext.h"

#define ABS(X)	 ((X)<0?-(X):(X))
#define MAX(A,B) ((A)>(B)?(A):(B))
#define MIN(A,B) ((A)<(B)?(A):(B))

char acc_dev_name[20];

int kb, kb_i, acc, acc_i;
struct pollfd polldat[2];

#define AXES 3
#define SYSFS "/sys/devices/platform/lf1000-aclmtr"

int cx;
int cy;
char text[100];

int init_input ()
{
	// Keyboard
	kb = open_input_device(KB_NAME);
	if (kb < 0)
	{
		perror("Can't find " KB_NAME);
		return 1;
	}
	// Input Event 
	if (find_input_device (ACC_NAME, acc_dev_name) < 0)
	{
		perror ("Can't find " ACC_NAME);
		return 1;
	}
	acc = open(acc_dev_name, O_RDONLY);
	if (acc < 0)
	{
		perror("failed to open " ACC_NAME);
		return 1;
	}
	kb_i = 0;
	polldat[0].fd = kb;
	polldat[0].events = POLLIN;
	acc_i = 1;
	polldat[1].fd = acc;
	polldat[1].events = POLLIN;
	return 0;
}

int main (int argc, char **argv)
{
	if (init_fb ())
		return 1;
	cy=vsize/2;
	cx=hsize/2;
	if (init_input ())
		return 1;
	if (init_drawtext ())
		return 2;
	cls ();

	int in_loop = 1;
	int i, j;
	int raw[AXES];
	struct input_event ev[64];

	// prime the pump
	int abs[5];
	ioctl (acc, EVIOCGABS(ABS_X), abs); raw[0]=abs[0];
	ioctl (acc, EVIOCGABS(ABS_Y), abs); raw[1]=abs[0];
	ioctl (acc, EVIOCGABS(ABS_Z), abs); raw[2]=abs[0];

	while (in_loop) {
		if (poll (polldat, 2, 0) <= 0)
			continue;
		if (polldat[kb_i].revents) // Keyboard
		{
			int rd = read(kb, ev, sizeof(struct input_event));
			if (rd == sizeof (struct input_event) && ev[0].type == EV_KEY &&
			    (ev[0].code == KEY_A || ev[0].code == KEY_ESC))
				if (ev[0].value == 1)
					in_loop = 0;
		}
		if (polldat[acc_i].revents) // Accel
		{
			int rd = read(acc, ev, sizeof(struct input_event) * 64);
			for (i = 0; i < rd / sizeof(struct input_event); i++)
			{
				double d;
				switch (ev[i].type)
				{
				case EV_SYN:
					d = sqrt(raw[0]*raw[0]+
						 raw[1]*raw[1]+
						 raw[2]*raw[2]);
					sprintf (text, "%3d %3d %3d : %5.2f", 
						 raw[0], raw[1], raw[2], d);
					drawtext (cx-8*strlen(text)/2, cy, text);
					break;
				case EV_ABS:
					j = ev[i].code;
					if (j>=0 && j<AXES)
						raw[j] = ev[i].value;
					break;
				}
			}
		}
	}
	cls();
	return 0;
}

