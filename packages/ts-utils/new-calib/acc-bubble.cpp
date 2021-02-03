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
#include "drawtext.h"
#include "OpenInput.h"
#include "MaxMin.h"
#include "Histo.h"
#include "Stats.h"

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

void draw_circle (int x0, int y0, int radius, int color)
{
	int i, i1;
	int x=x0;
	int y=y0;
	int r2=radius*radius;
	for (i=0; i<=radius; i++)
	{
		i1 = (int)sqrt(r2-i*i);
		dot (x0+i, y0+i1, color);
		dot (x0+i, y0-i1, color);
		dot (x0-i, y0+i1, color);
		dot (x0-i, y0-i1, color);
		dot (x0+i1, y0+i, color);
		dot (x0+i1, y0-i, color);
		dot (x0-i1, y0+i, color);
		dot (x0-i1, y0-i, color);
	}
}

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


int averaging = 0;
int main (int argc, char **argv)
{
	char *text[4] = { "Raw", "Raw + Low Pass", "Cooked", "Cooked + Low Pass" };
	enum MODE { RAW, RAW_LP, COOKED, COOKED_LP } mode=COOKED_LP;

	system ("acc-cooked.sh"); averaging = 1; 
	system ("echo 1 > /sys/devices/platform/lf1000-aclmtr/tick"); // keep the fun rolling!

	if (init_fb ())
		return 1;
	if (init_drawtext ())
		return 1;
	cy=vsize/2;
	cx=hsize/2;
	if (init_input ())
		return 1;
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
			    (ev[0].code == KEY_A || ev[0].code == KEY_ESC) &&
			    ev[0].value == 1)
			{
				mode = (enum MODE)((mode + 1) %4);
				switch (mode)
				{
				case RAW:       system ("acc-raw.sh"); averaging = 0; break;
				case RAW_LP:    system ("acc-raw.sh"); averaging = 1; break;
				case COOKED:    system ("acc-cooked.sh"); averaging = 0; break;
				case COOKED_LP: system ("acc-cooked.sh"); averaging = 1; break;
				}
			}
		}
		if (polldat[acc_i].revents) // Accel
		{
			int x, y;
			int rd = read(acc, ev, sizeof(struct input_event) * 64);
			for (i = 0; i < rd / sizeof(struct input_event); i++)
			{
				double d;
				const int gain=30;
				switch (ev[i].type)
				{
				case EV_SYN:
					x = averaging ? (4*x+gain*raw[0])/5 :
						gain*raw[0];
					y = averaging ? (4*y+gain*raw[1])/5 :
						gain*raw[1];
					cls ();
					drawtext (0,0, text[mode]);
					draw_circle (cx, cy, 40, 0x808080);
					draw_circle (cx, cy, 100, 0x000000);
					draw_circle (cx+x, cy+y,
						     30, 0xff8080);
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

