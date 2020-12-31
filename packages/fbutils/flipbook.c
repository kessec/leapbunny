/* Draw pngs as a flip-book movie */
/* Code generally lifted from Dave's drawtext */

#define ORDER	51
#define SCRX	320
#define SCRY	240

#define _FILE_OFFSET_BITS	64	// for correct off_t type

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <stdio.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <string.h>

#include <linux/lf1000/mlc_ioctl.h>

/* Added for png support */
#include "readpng.h"
#include "formats.h"

// We just assume hstride is 4 all over the place, so why pretend otherwise for performance?
#define HSTRIDE	4
#define VSTRIDE (SCRX*HSTRIDE)
int fb, bg, work;

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

	int base, fbsize;
	int i,j;
	unsigned long width, height, bpr;
	int bpp, fmt;
	unsigned char *png_data[ORDER] = { NULL };
	FILE *pngfile;

	fb = -1;
	bg = -1;
	work = -1;
	
	if (argc<2)
	{
		fprintf (stderr, "Usage: flipbook DIRECTORY\n");
		return 1;
	}

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

	// Save the background
	scrcpy (bg, fb);

	//
	// Open PNGs
	// 
	for (i=1; i<=51; i++)
	{
		char name[128];
		sprintf (name, "%s/Particle_%04d.png", argv[1], i);
		if (!(pngfile = fopen(name, "rb"))) {
			fprintf (stderr, "%s: ", name);
			perror("can't open intput file.");
			return 1;
		}
		if (readpng_init(pngfile, &width, &height) == 0) {
			/* We have a png. Decode it. */
			png_data[i-1] = readpng_get_image(2.2, &bpp, &bpr);
			if (!png_data[i-1]) {
				fprintf(stderr, "unable to decode PNG image\n");
				goto cleanup;
			}	

			/* If bpp is 3, assume B8G8R8 which is 0xC653.  If bpp is 4,
			 * assume A8B8G8R8 which is 0x8653 */
			// printf ("For %s we got bpp=%d\n", name, bpp);
			if (bpp == 3) {
				fmt = 0xC653;
				fprintf(stderr, "Unsupported bytes per pixel: %d\n", bpp);
				goto cleanup;
			} else if (bpp == 4) {
				fmt = 0x8653;
			} else {
				fprintf(stderr, "Unsupported bytes per pixel: %d\n", bpp);
				goto cleanup;
			}
		}
	}
		
	// draw the bezier now
	i = 0;
	while (1)
	{
		const int frames = 60;
		struct timeval t0, t1;
		gettimeofday (&t0, 0);
		for (j=0; j<frames; j++)
		{
			// Clear out work space
			scrcpy (work, bg);
			// Render our image... ha ha ha... with alpha blending somehow... ha ha ha
			unsigned char *s = png_data[i++ % ORDER];
			unsigned char *d = (unsigned char *)work;
			int x, y;
			for (y=0; y<SCRY; y++)
			{
				for (x=0; x<SCRX; x++)
				{
					if (s[4]==0)
					{
						// Transparent
						s += 4;
						d += 4;
					}
					else
					{
						// Blended 
						unsigned char r0,g0,b0, r1,g1,b1;
						int a;
						r0 = *s++;
						g0 = *s++;
						b0 = *s++;
						a = *s++;
						r1 = *d++;
						g1 = *d++;
						b1 = *d--;
						d--;
						*d++ = ((r0 * a)+(r1 * (256-a))) >> 8;
						*d++ = ((g0 * a)+(g1 * (256-a))) >> 8;
						*d++ = ((b0 * a)+(b1 * (256-a))) >> 8;
						d++;
					}
				}
			}
			// Refresh screen
			scrcpy (fb, work);
			// Prep to poll for vsync
			// ioctl(layer, MLC_IOCTDIRTY, 0);
			// Poll
			// while (ioctl(layer, MLC_IOCQDIRTY, 0))
			// 	usleep (10000);
			
			// Just sleep a bit.  Better to use timer, but not worth it yet.
			usleep (1000000/20);
		}
		if (argc > 2)
		{
			gettimeofday (&t1, 0);
			float delta = t1.tv_sec - t0.tv_sec + 1e-6*(t1.tv_usec - t0.tv_usec);
			float fps = frames / delta;
			printf ("%5.2f fps\n", fps);
		}
	}		
		
cleanup:
	if (fb >= 0) close(fb);
	if (layer >= 0) close(layer);
	return 0;
}
