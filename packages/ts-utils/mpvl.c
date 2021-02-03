// Mister Pencil Very Light: use both input event system and tslib, if possible

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

#include <linux/lf1000/mlc_ioctl.h>

#include <tslib.h>

#define LAYER	"/dev/layer0"
#define MLC	"/dev/mlc"
#define TS_NAME	"LF1000 touchscreen interface"
#define KB_NAME	"LF1000 Keyboard"

#define TSLIB_COLOR	0xff0000	// RED
#define INPUT_COLOR	0x0000FF	// BLUE

int hstride = -1, vstride = -1;
int base, fbsize, bufsize;
int ssi, hsize, vsize;
void *fb = (void *)-1;
struct screensize_cmd ss;
int layer, mlc;
unsigned char *dc;
unsigned long *dl;

char ts_dev_name[20];

// Borrow some code to look up event devices
#include "open-input.c"

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

enum maskval { m_input=1, m_tslib=2 } mask = m_input | m_tslib;

int main (int argc, char **argv)
{
	printf ("%s: Mr Pencil Very Light\n", argv[0]);
	printf ("Optionally pass in 1=input, 2=tslib, 3=both (both)\n");
	printf ("Press A to clear screen, B to exit\n");
	printf ("input event in BLUE; tslib in RED\n");
	if (argc > 1)
		mask = (enum maskval) atoi (argv[1]);

	// Set up frame buffer
	layer = open(LAYER, O_RDWR|O_SYNC);
	if(layer < 0)
	{
		perror("failed to open layer device");
		return 1;
	}
	mlc = open(MLC, O_RDWR);
	if(mlc < 0)
	{
		perror("failed to open mlc device");
		return 1;
	}

	/* get the base address for this layer's frame buffer */
	base = ioctl(layer, MLC_IOCQADDRESS, 0);
	if(base == -EFAULT) {
		perror("get_address ioctl failed");
		return 1;
	}
	/* figure out the size of the frame buffer */
	fbsize = ioctl(layer, MLC_IOCQFBSIZE, 0);
	if(fbsize < 0) {
		perror("get_fbsize ioctl failed");
		return 1;
	}
	hstride = ioctl(layer, MLC_IOCQHSTRIDE, 0);
	if(hstride < 0) {
		perror("get_hstride ioctl failed");
		return 1;
	}

	vstride = ioctl(layer, MLC_IOCQVSTRIDE, 0);
	if(vstride < 0) {
		perror("get_vstride ioctl failed");
		return 1;
	}
	ssi = ioctl (mlc, MLC_IOCGSCREENSIZE, &ss);
	if(ssi < 0) {
		perror("get screensize ioctl failed");
		return 1;
	}
	hsize = ss.width;
	vsize = ss.height;

	// Whew! Resize the screen and view port
	if (1 || vstride != hsize * hstride)
	{
		// Need to reprogram this guy
		vstride = hsize * hstride;
		int x = ioctl(layer, MLC_IOCTVSTRIDE, vstride);
		if(x < 0) {
			perror("set vstride ioctl failed");
			return 1;
		}
		struct position_cmd pos;
		pos.top = 0;
		pos.left = 0;
		pos.right = hsize;
		pos.bottom = vsize;
		x = ioctl(layer, MLC_IOCSPOSITION, &pos);
		if(x < 0) {
			perror("set positio ioctl failed");
			return 1;
		}
		x = ioctl(layer, MLC_IOCTDIRTY, 1);
		if(x < 0) {
			perror("set dirty ioctl failed");
			return 1;
		}
	}

	fb = mmap(0, fbsize, PROT_READ | PROT_WRITE, MAP_SHARED, layer, 
				   (unsigned int)base);
	if(fb == MAP_FAILED) {
		perror("mmap() failed");
		return 1;
	}

	printf ("fb=%08lx hstride=%d vstride=%d fbsize=%d ss.w=%d ss.h=%d\n", 
		(unsigned long)fb, hstride, vstride, fbsize,
		ss.width, ss.height);

 	dc = (unsigned char*)fb;

	// Clear screen
	memset (dc, 0xff, hstride*hsize*vsize);

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
			// Pen down?  (but skip if user didn't want it)
			if (samp[i].pressure && (mask & m_tslib))
			{
				fatdot (samp[i].x, samp[i].y, TSLIB_COLOR);
				if (tlx != -1 && tly != -1)
					line (tlx, tly, samp[i].x, samp[i].y, TSLIB_COLOR);
				tlx = samp[i].x;
				tly = samp[i].y;
			}
			else
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
							memset (dc, 0xff, hstride*hsize*vsize);
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
			if (!(mask & m_input))
				continue;	// Skip if user didn't want it
			for (i = 0; i < rd / sizeof(struct input_event); i++)
			{
				switch (ev[i].type)
				{
				case EV_SYN: 
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

