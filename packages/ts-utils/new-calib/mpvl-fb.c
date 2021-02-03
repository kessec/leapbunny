// Mister Pencil Very Light: use both input event system and tslib, if possible

#define _FILE_OFFSET_BITS	64

#include <stdint.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <unistd.h>
#include <poll.h>

#include <linux/fb.h>
#include <linux/lf1000/lf1000fb.h>

#include <tslib.h>

#define TS_NAME	"LF1000 touchscreen interface"
#define KB_NAME	"LF1000 Keyboard"

#define FB_NAME "/dev/fb0"

#define TSLIB_COLOR	0xff0000	// RED
#define INPUT_COLOR	0x0000FF	// BLUE

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
int hstride, vstride;
int hsize, vsize;
void *fb = (void *)-1;
int fbdev;
unsigned char *dc;

char ts_dev_name[20];

// Borrow some code to look up event devices
#include "open-input.c"

void cls ()
{
	memset (dc, 0xff, hstride*hsize*vsize);
}

void inline unsafedot (int x, int y, long color)
{
	unsigned long *d = (unsigned long *)&dc[y*vstride+x*hstride];
	*d = color;
}
void inline dot (int x, int y, long color)
{
	if (x >= 0 && y>= 0 && x<hsize && y<vsize)
	{
		unsigned long *d = (unsigned long *)&dc[y*vstride+x*hstride];
		*d = color;
	}
}

void fatdot (int x, int y, long color)
{
	if (x >= 0 && y>= 0 && x<hsize && y<vsize)
	{
		unsafedot (x, y, color);
		if (x+1<hsize)
			unsafedot (x+1, y, color);
		if (y+1<vsize)
		{
			unsafedot (x, y+1, color);
			if (x+1<hsize)
				unsafedot (x+1, y+1, color);
		}
	}
}

void line (int x0, int y0, int x1, int y1, long color)
{
	int sx=1, sy=1;
	int dx=x1-x0;
	int ax=dx; if (ax<0) { sx=-1; ax=-ax; }
	int dy=y1-y0;
	int ay=dy; if (ay<0) { sy=-1; ay=-ay; }
	if (ax>=ay && ax>0)
	{
		int i;
		int d=(65536*dy)/ax;
		int s=65536*y0;
		for (i=0; i<=ax; i++)
		{
			int ss = s>>16;
			dot (x0+sx*i, ss, color);
			s += d;
		}
	}
	else if (ax<ay && ay>0)
	{
		int i;
		int d=(65536*dx)/ay;
		int s=65536*x0;
		for (i=0; i<=ay; i++)
		{
			int ss = s>>16;
			dot (ss, y0+sy*i, color);
			s += d;
		}
	}
}

int main (int argc, char **argv)
{
	printf ("%s: Mr Pencil Very Light\n", argv[0]);
	printf ("Press A to clear screen, B to exit\n");
	printf ("input event in BLUE; tslib in RED\n");
	int print2 = 0;
	if (argc > 1)
		print2 = 1;

	// Set up frame buffer
	fbdev = open("/dev/fb0", O_RDWR|O_SYNC);
	if(fbdev < 0)
	{
		perror("failed to open fb device");
		return 1;
	}

	// Ask for screen res in hacky way
	FILE *p = fopen ("/sys/devices/platform/lf1000-dpc/xres", "r");
	if (!p)
	{
		perror("failed to open /sys/devices/platform/lf1000-dpc/xres");
		return 1;
	}
	fscanf (p, "%d", &hsize);
	fclose (p);
	p = fopen ("/sys/devices/platform/lf1000-dpc/yres", "r");
	if (!p)
	{
		perror("failed to open /sys/devices/platform/lf1000-dpc/yres");
		return 1;
	}
	fscanf (p, "%d", &vsize);
	fclose (p);

	/* Get fixed screen information */
	if (ioctl(fbdev, FBIOGET_FSCREENINFO, &finfo)) {
		perror("error reading fixed information.");
		return 1;
	}

	/* Get variable screen information */
	if (ioctl(fbdev, FBIOGET_VSCREENINFO, &vinfo)) {
		perror("error reading variable information.");
		return 1;
	}

	printf ("fixed: line_length=%d mmio_len=%d smem_len=%d\n",
		finfo.line_length, finfo.mmio_len, finfo.smem_len);
	hstride = vinfo.bits_per_pixel>>3;
	vstride = hsize * hstride; // finfo.line_length;
	vinfo.xres = hsize;
	vinfo.yres = vsize;
	vinfo.width = hsize;
	vinfo.height = vsize;
	vinfo.blue.offset=0;
	vinfo.green.offset=8;
	vinfo.red.offset=16;
	printf ("vari: res(%d,%d) vres(%d,%d) offset(%d,%d) bbp=%d w,h=%d,%d\n",
		vinfo.xres, vinfo.yres, 
		vinfo.xres_virtual, vinfo.yres_virtual, 
		vinfo.xoffset, vinfo.yoffset, 
		vinfo.bits_per_pixel,
		vinfo.width, vinfo.height);

	if (ioctl(fbdev, FBIOPUT_VSCREENINFO, &vinfo))
	{
		perror("error setting variable info");
		return 1;
	}
	

	struct lf1000fb_position_cmd cmd;
	cmd.left = cmd.top = 0;
	cmd.right=hsize;
	cmd.bottom=vsize;
	if (ioctl(fbdev, LF1000FB_IOCSPOSTION, &cmd))
	{
		perror("error setting lf1000 position");
		return 1;
	}


	fb = (char *)mmap(0, finfo.smem_len,
			  PROT_READ | PROT_WRITE, MAP_SHARED, fbdev, 0);
	if (fb == MAP_FAILED) {
		perror("mmap() failed");
		return 1;
	}

	printf ("fb=%p hstride=%d vstride=%d w=%d h=%d\n", 
		fb, hstride, vstride, hsize, vsize);

 	dc = (unsigned char*)fb;

	// Clear screen
	cls ();

	// Keyboard
	int kb = open_input_device(KB_NAME);
	if (kb < 0)
	{
		perror("Can't find " KB_NAME);
		return 1;
	}
	// Input Event 
	if (find_input_device (TS_NAME, ts_dev_name) < 0)
	{
		perror ("Can't find " TS_NAME);
		return 1;
	}
	int ts = open(ts_dev_name, O_RDONLY);
	if (ts < 0)
	{
		perror("failed to open ts device");
		return 1;
	}
	struct pollfd polldat[2] = {{ ts, POLLIN }, {kb, POLLIN }};
	// TSLIB
	struct tsdev *tsl = ts_open(ts_dev_name, 1);
	if (!tsl) {
		perror("ts_open");
		return 1;
	}

	if (ts_config(tsl)) {
		perror("ts_config");
		return 1;
	}

	// The main event loop
	while (1) {
		static int tlx=-1, tly=-1;
		static int ilx=-1, ily=-1;
		static int x=-1, y=-1, p=-1, state=-1;
		struct ts_sample samp[64];
		struct input_event ev[64];
		int i;
		
		// tslib have something for us?  If so, draw it
		int ret = ts_read(tsl, samp, 64);
		for (i=0; i<ret; i++)
		{
			if (print2)
				printf ("\t\ttslib: %d,%d,%d;\n", samp[i].x, samp[i].y, samp[i].pressure);
			// Pen down?  (but skip if user didn't want it)
			{
				fatdot (samp[i].x, samp[i].y, TSLIB_COLOR);
				if (tlx != -1 && tly != -1)
					line (tlx, tly, samp[i].x, samp[i].y, TSLIB_COLOR);
				tlx = samp[i].x;
				tly = samp[i].y;
			}
			if (!samp[i].pressure)
			{
				tlx = tly = -1;
			}
		}
		
		// input event system: poll keyboard[1] and ts[0]
		if (poll (polldat, 2, 0) > 0)
		{
			if (polldat[1].revents) // Keyboard
			{
				int rd = read(kb, ev, sizeof(struct input_event));
				if (rd == sizeof (struct input_event))
				{
					if (ev[0].type == EV_KEY)
					{
						if (ev[0].code == KEY_A)
						{
							cls ();
							if (print2) printf ("--------------\n");
						}
						else if (ev[0].code == KEY_B)
							return 0;
					}
				}
			}
			if (!polldat[0].revents)  // Not TS Input
				continue;

			// Must be TS input...
			int rd = read(ts, ev, sizeof(struct input_event) * 64);
			if (rd < (int) sizeof(struct input_event)) {
				perror("\nmyinput: error reading");
				return 1;
			}
			for (i = 0; i < rd / sizeof(struct input_event); i++)
			{
				switch (ev[i].type)
				{
				case EV_SYN: 
					if (print2)
						printf ("input: %d,%d,%d;\n", x, y, p);
					if (state)
					{
						fatdot(x+2, y, INPUT_COLOR);
						if (ilx != -1 && ily != -1)
							line (ilx+2, ily, x+2, y, INPUT_COLOR);
						ilx = x;
						ily = y;
					}
					break;
				case EV_ABS:
					switch (ev[i].code)
					{
					case ABS_X: x=ev[i].value; break;
					case ABS_Y: y=ev[i].value; break;
					case ABS_PRESSURE: p=ev[i].value; break;
					}
					break;
				case EV_KEY:
					state = ev[i].value;
					if (state)
						ilx = ily = -1;
					break;
				}
			}
		}
	}
}

