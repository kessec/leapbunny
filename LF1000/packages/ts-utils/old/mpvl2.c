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

#include <linux/lf1000/mlc_ioctl.h>

#include <tslib.h>
// #include <tslib-private.h>

#define LAYER	"/dev/layer0"
#define MLC	"/dev/mlc"
#define TS	"/dev/input/event3"
#define KB	"/dev/input/event1"

int hstride = -1, vstride = -1;
int base, fbsize, bufsize;
int ssi, hsize, vsize;
void *fb = (void *)-1;
struct screensize_cmd ss;
int layer, mlc;
unsigned char *dc;
unsigned long *dl;


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

int use_tslib = 0;

int main (int argc, char **argv)
{
	if (argc>1 && atoi(argv[1]))
		use_tslib=1;
	printf ("Using %s\n", use_tslib ? "tslib" : TS);

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

	/* Touch screen */
	if (use_tslib)
	{
		struct tsdev *tsl = ts_open(TS, 0);
		if (!tsl) {
			perror("ts_open");
			return 1;
		}

		if (ts_config(tsl)) {
			perror("ts_config");
			return 1;
		}

		while (1) {
			static int lx=-1, ly=-1;
			struct ts_sample samp;
			int ret = ts_read(tsl, &samp, 1);
			if (ret < 0) {
				perror("ts_read");
				return 1;
			}
			if (ret != 1)
				continue;
			if (samp.pressure)
			{
				fatdot (samp.x, samp.y, 0x0);
				if (lx != -1 && ly != -1)
					line (lx, ly, samp.x, samp.y, 0x0);
				lx = samp.x;
				ly = samp.y;
			}
			else
			{
				lx = ly = -1;
			}
		}
	}
	else
	{
		int ts = open(TS, O_RDONLY);
		if (ts < 0)
		{
			perror("failed to open ts device");
			return 1;
		}
		while (1) {
			int i;
			struct input_event ev[64];
			static int lx=-1, ly=-1;
			static int x=-1, y=-1, p=-1, state=-1;
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
					if (state)
					{
						fatdot(x, y, 0x0);
						if (lx != -1 && ly != -1)
							line (lx, ly, x, y, 0x0);
						lx = x;
						ly = y;
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
						lx = ly = -1;
					break;
				}
			}
		}
	}
}

