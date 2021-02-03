// Accelerometer Calibration

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

#define ACC_NAME	"LF1000 Accelerometer"
#define KB_NAME	"LF1000 Keyboard"

#include "fb.h"
#include "OpenInput.h"
#include "Stats.h"
#include "drawtext.h"

#define ABS(X)	 ((X)<0?-(X):(X))
#define MAX(A,B) ((A)>(B)?(A):(B))
#define MIN(A,B) ((A)<(B)?(A):(B))

char acc_dev_name[20];

int kb, kb_i, acc, acc_i;
struct pollfd polldat[2];
FILE *flog = NULL;

int setting_bias[3];
#define AXES 3
#define SYSFS "/sys/devices/platform/lf1000-aclmtr"

int cx;
int cy;
char text[100];

int get_sysfs (const char *sysfs)
{
	char buf[100];
	sprintf (buf, "%s/%s", SYSFS, sysfs);
	FILE *f;
	f = fopen (buf, "r");
	if (!f) { printf ("%s: ", sysfs); fflush (stdout); perror ("get_sysfs"); exit (1); }
	int x;
	if (fscanf (f, "%d", &x) < 1) { printf ("scanf failed"); exit (1); }
	fclose (f);
	return x;
}

void set_sysfs (const char *sysfs, int value)
{
	char buf[100];
	sprintf (buf, "%s/%s", SYSFS, sysfs);
	FILE *f;
	f = fopen (buf, "w");
	if (!f) { printf ("%s: ", sysfs); fflush (stdout); perror ("set_sysfs"); exit (1); }
	fprintf (f, "%d\n", value);
	fclose (f);
}

void set_sysfs3 (const char *sysfs, int v1, int v2, int v3)
{
	char buf[100];
	sprintf (buf, "%s/%s", SYSFS, sysfs);
	FILE *f;
	f = fopen (buf, "w");
	if (!f) { printf ("%s: ", sysfs); fflush (stdout); perror ("set_sysf3"); exit (1); }
	fprintf (f, "%d %d %d\n", v1, v2, v3);
	fclose (f);
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

// Return <0 for abort, >0 for retry, and 0 for Success
// Set just_delays to 1 to reduce logging and expect more input w/o pen-up
int get_bias (const char *prompt, double *res)
{
	printf ("Entering get_bias with prompt:%s\n", prompt);
	if (flog)
		fprintf (flog, "Entering get_bias with prompt:%s\n", prompt);
	cls ();
	int want=100;
	int in_loop = 1;
	int last_seconds = 0;
	int too_noisy = 0;
	CStats s[AXES];
	int raw[AXES];
	enum state { TELL, WAIT, RELEASE, STALL, COLLECTING } state = TELL;
	int i, j;
	struct input_event ev[64];
	int stall_count = 25;

	// prime the pump
	int abs[5];
	ioctl (acc, EVIOCGABS(ABS_X), abs); raw[0]=abs[0];
	ioctl (acc, EVIOCGABS(ABS_Y), abs); raw[1]=abs[0];
	ioctl (acc, EVIOCGABS(ABS_Z), abs); raw[2]=abs[0];

	while (in_loop) {
		switch (state)
		{
		case TELL:
			cls ();
			sprintf (text, "%s", prompt);
			drawtext (cx-8*strlen(text)/2, cy-20, text);
			sprintf (text, "Press A or ESC when ready");
			drawtext (cx-8*strlen(text)/2, cy+20, text);
			state = WAIT;
			break;
		case WAIT:
		case RELEASE:
			if (poll (polldat, 2, 0) <= 0)
				break;
			if (polldat[kb_i].revents) // Keyboard
			{
				int rd = read(kb, ev, sizeof(struct input_event));
				if (rd == sizeof (struct input_event) && ev[0].type == EV_KEY &&
				    (ev[0].code == KEY_A || ev[0].code == KEY_ESC))
				{
					if (state == WAIT && ev[0].value == 1)
					{
						state = RELEASE;
						cls ();
						sprintf (text, "Release button");
						drawtext (cx-8*strlen(text)/2, cy, text);
						if (flog)
							fprintf (flog, "Wait for release\n");
					}
					else if (state == RELEASE && ev[0].value == 0)
					{
						state = STALL;
						cls ();
						sprintf (text, "Collecting...");
						drawtext (cx-8*strlen(text)/2, cy, text);
						if (flog)
							fprintf (flog, "Stall to stabilize\n");
					}
				}
			}
			if (polldat[acc_i].revents) // Accel
				int rd = read(acc, ev, sizeof(struct input_event) * 64); // toss
			break;
		case STALL:
		case COLLECTING:
			if (poll (polldat, 2, 0) <= 0)
				break;
			if (polldat[kb_i].revents) // Keyboard
				int rd = read(kb, ev, sizeof(struct input_event)); // toss
			if (polldat[acc_i].revents) // Accel
			{
				int rd = read(acc, ev, sizeof(struct input_event) * 64);
				for (i = 0; i < rd / sizeof(struct input_event); i++)
				{
					switch (ev[i].type)
					{
					case EV_SYN:
						if (flog)
						{
							fprintf (flog, "x=%d y=%d z=%d\n", raw[0], raw[1], raw[2]);
						}
						if (state == COLLECTING)
						{
							s[0].Add(raw[0]);
							s[1].Add(raw[1]);
							s[2].Add(raw[2]);
							if (s[0].N() >= want)
								in_loop = 0;
						}
						else // (state == STALL)
						{
							if (--stall_count <= 0)
							{
								state = COLLECTING;
								if (flog)
									fprintf (flog, "Start collecting\n");
							}
						}
						break;
					case EV_ABS:
						j = ev[i].code;
						if (j>=0 && j<AXES)
							raw[j] = ev[i].value;
						break;
					}
				}
			}
			break;
		default:
			break;
		}
	}
	if (flog)
	{
		for (i=0; i<AXES; i++)
			fprintf (flog, " %7.2f (%6.2f)", s[i].Mean(), s[i].Stdev());
		fprintf (flog, "\n\n");
	}
	for (i=0; i<AXES; i++)
		res[i] = s[i].Mean();
	return 0;
}

int write_mfgdata_acc (const char *filename)
{
	FILE *f = fopen (filename, "w");
	if (!f)
	{
		perror ("write_mfgdata_acc: Can't open for output");
		return 1;
	}
	// See libMfgData.h for order of this file
	fprintf (f, "%d ", setting_bias[0]);
	fprintf (f, "%d ", setting_bias[1]);
	fprintf (f, "%d\n", setting_bias[2]);
	fchmod (fileno(f), 0666);
	fclose (f);
	return 0;
}

int write_settssh (const char *filename)
{
	FILE *f = fopen (filename, "w");
	if (!f)
	{
		perror ("write_settssh: Can't open for output");
		return 1;
	}
	fprintf (f, "SYS=/sys/devices/platform/lf1000-touchscreen\n");
	/*
	fprintf (f, "echo %d > $SYS/max_tnt_down\n", setting_max_tnt_down);
	fprintf (f, "echo %d > $SYS/min_tnt_up\n", setting_min_tnt_up);
	fprintf (f, "echo %d > $SYS/delay_in_us\n", setting_delay_in_us);
	fprintf (f, "echo %d > $SYS/y_delay_in_us\n", setting_y_delay_in_us);
	fprintf (f, "echo %d > $SYS/tnt_delay_in_us\n", setting_tnt_delay_in_us)
;
	*/
	fchmod (fileno(f), 0777);
	fclose (f);
	return 0;
}

int main (int argc, char **argv)
{
	int i;
	printf ("%s: accelerometer calibration program\n", argv[0]);
	if (init_fb ())
		return 1;
	cy=vsize/2;
	cx=hsize/2;
	if (init_input ())
		return 1;
	if (init_drawtext ())
		return 2;
	int a = get_sysfs ("average");
	flog = fopen ("/var/log/acc-calib.log", "w");
	if (!flog)
	{
		perror ("Can't open /var/log/acc-calib.log");
		return 3;
	}
	fprintf (flog, "acc-calib.log: fgrep -v X2= to remove cruft\n");

	set_sysfs ("tick", 1);
	set_sysfs ("rate", 1);
	set_sysfs ("average", 1);
	set_sysfs3 ("bias", 0, 0, 0);
	
	double s1[AXES], s2[AXES];
	get_bias ("Place device on a level surface", &s1[0]);
	get_bias ("Rotate device 180 deg clockwise", &s2[0]);
	cls ();
	sprintf (text, "Done");
	drawtext (cx-8*strlen(text)/2, cy, text);

	// Compute vector
	double s[AXES];
	for (i=0; i<AXES; i++)
		s[i] = (s1[i] + s2[i])/2;
	// printf ("s = %7.2f %7.2f %7.2f\n", s[0], s[1], s[2]);
	s[2] -= -15.5;
	printf ("bias=%7.2f %7.2f %7.2f\n", s[0], s[1], s[2]);
	if (flog)
		fprintf (flog, "bias=%7.2f %7.2f %7.2f\n", s[0], s[1], s[2]);

	set_sysfs ("tick", 0);
	for (i=0; i<AXES; i++)
		setting_bias[i] = (int)(s[i] * 10);
	set_sysfs3 ("bias", setting_bias[0], setting_bias[1], setting_bias[2]);
	write_mfgdata_acc ("/tmp/mfgdata-aclbias-data");
	return 0;
}
