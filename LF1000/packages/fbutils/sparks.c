/* Draw some sparks along a special quintic bezier */
/* Code generally lifted from Dave's drawtext */

#define ORDER	5
#define SPARKS	100
#define SCRX	320
#define SCRY	240

const int c[ORDER][2] = {
	{ 100, 108 },
	{ 54,   15 },
	{ 180, -20 },
	{ 171,  89 },
	{ 243,  79 },
};

#define bezier(p,i)			    \
	1*c[0][i]*(1-p)*(1-p)*(1-p)*(1-p) + \
	4*c[1][i]*(1-p)*(1-p)*(1-p)*(p) + \
	6*c[2][i]*(1-p)*(1-p)*(p)*(p) + \
	4*c[3][i]*(1-p)*(p)*(p)*(p) + \
	1*c[4][i]*(p)*(p)*(p)*(p)

#define bezier_x(p) bezier(p,0)
#define bezier_y(p) bezier(p,1)

#define _FILE_OFFSET_BITS	64	// for correct off_t type

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#include <linux/lf1000/mlc_ioctl.h>

// We just assume hstride is 4 all over the place, so why pretend otherwise for performance?
#define HSTRIDE	4
#define VSTRIDE (SCRX*HSTRIDE)
int fb, bg, work;

void dot (int x, int y, unsigned long color)
{
	if (x >= 0 && x < SCRX && y >= 0 && y < SCRY)
	{
		unsigned long *l = (unsigned long *)(work + x*HSTRIDE + y*VSTRIDE);
		*l = color;
	}
}

void adot (int x, int y, unsigned char s)
{
	if (x >= 0 && x < SCRX && y >= 0 && y < SCRY)
	{
		unsigned char *c = (unsigned char *)(work + x*HSTRIDE + y*VSTRIDE);
		*c = ((0xff * s)+(*c * (256-s))) >> 8;
		++c;
		*c = ((0xff * s)+(*c * (256-s))) >> 8;
		++c;
		*c = ((0xff * s)+(*c * (256-s))) >> 8;
	}
}

void scrcpy (int dst, int src)
{
	memcpy ((char *)dst, (char *)src, VSTRIDE * SCRY);
}


struct spark_data
{
	float x, y;
	float vx, vy;
	// unsigned long color;
	long state;
} *spark;

int main(int argc, char **argv)
{
	int layer = -1;
	int row = 0;
	int col = 0;

	int base, fbsize, bufsize;
	struct stat statbuf;
	int i,j,k,ds;
	unsigned char* s;
	unsigned char* d;
	int next = 0;
	
	//For the bezier
	float p = 1.0;

	fb = -1;
	bg = -1;
	work = -1;
	
	layer = open("/dev/layer0", O_RDWR|O_SYNC);
	if(layer < 0) {
		perror("failed to open layer device");
		return 1;
	}

	/* get the base address for this layer's frame buffer */
	base = ioctl(layer, MLC_IOCQADDRESS, 0);
	if(base == -EFAULT) {
		perror("get_address ioctl failed");
		goto cleanup;
	}

	/* figure out the size of the frame buffer */
	fbsize = ioctl(layer, MLC_IOCQFBSIZE, 0);
	if(fbsize < 0) {
		perror("get_fbsize ioctl failed");
		goto cleanup;
	}

	// HSTRIDE = ioctl(layer, MLC_IOCQHSTRIDE, 0);
	/* printf ("HSTRIDE=%d\n", HSTRIDE); */
	//if(HSTRIDE < 0) {
	//	perror("get_HSTRIDE ioctl failed");
	//	goto cleanup;
	//}
	//vstride = SCRX * HSTRIDE;

	fb = (int)mmap(0, fbsize, PROT_READ | PROT_WRITE, MAP_SHARED, layer, 
				   (unsigned int)base);
	if(fb < 0) {
		perror("mmap() failed");
		goto cleanup;
	}

	bg = (int)malloc(240*VSTRIDE);
	if (bg == 0) {
		perror("bg malloc() failed");
		goto cleanup;
	}
	work = (int)malloc(240*VSTRIDE);
	if (work == 0) {
		perror("work malloc() failed");
		goto cleanup;
	}
	spark = (struct spark_data *)malloc(SPARKS*sizeof(struct spark_data));
	for (i=0; i<SPARKS; i++)
		spark[i].state = 0;

	// Save the background
	scrcpy (bg, fb);

	// draw the bezier now
	for (j=0;; j++)
	{
		ioctl(layer, MLC_IOCTDIRTY, 0);
		// Clear out work space
		scrcpy (work, bg);
		// Create a new spark
		spark[next].x = bezier_x(p);
		spark[next].y = bezier_y(p);
		spark[next].vx = ((rand()&32767)-16384)/8192.0;
		spark[next].vy = ((rand()&32767)-16384)/8192.0;
		spark[next].state = 250;
		//printf ("next=%d x=%f y=%f vx=%f vy=%f\n", next,
		//	spark[next].x, spark[next].y, 
		//	spark[next].vx, spark[next].vx);
		next = (next + 1) % SPARKS;
		// Draw all sparks
		for (i=0; i<SPARKS; i++)
		{
			if (spark[i].state > 0)
			{
				int x = (int)spark[i].x;
				int y = (int)spark[i].y;
				int s = spark[i].state;
				adot (x, y, s);
				adot (x+1, y, s);
				adot (x-1, y, s);
				adot (x, y+1, s);
				adot (x, y-1, s);
				spark[i].state -= 5;
				spark[i].x += spark[i].vx;
				spark[i].y += spark[i].vy;
				spark[i].vx *= .95;
				spark[i].vy *= .95;
				spark[i].vy += .05;
			}
		}
		// Refresh screen
		scrcpy (fb, work);
		// Again
		p -= 0.01;
		if (p<0)
			p = 1.0;
		while (ioctl(layer, MLC_IOCQDIRTY, 0))
			usleep (1000);
		// usleep (1000000/100);
	}
		
cleanup:
	if (fb >= 0) close(fb);
	if (layer >= 0) close(layer);
	return 0;
}
