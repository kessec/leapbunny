#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

// r|w device #512blocks

#define BLOCK	512
char buf[BLOCK];

#define MAX_HISTO 2000
int h[MAX_HISTO+1];

struct timeval t0, t1;

void init ()
{
	int i;
	for (i=0; i<=MAX_HISTO; i++)
		h[i] = 0;
}

void show (int expected_runs)
{
	int i, j;
	int total=0;
	int runs=0;
	for (i=0, j=1; j<=MAX_HISTO; j*=10)
	{
		for (; i<MAX_HISTO && i<j*10; i+=j)
		{
			int k, t=0;
			for (k=i; k<i+j; k++)
			{
				t += h[k];
				total += h[k]*k;
			}
			runs += t;
			printf ("%4d-%4d: ", i, i+j-1);
			if (t)
				printf ("%5d   %9.6f%%\n", t, 100.0*t/expected_runs);
			else
				printf ("\n");
		}
	}
	printf ("Overflow:  %d\n", h[MAX_HISTO]);
	printf ("Overall:   %dms for %d runs\n", total, runs);
}

void inline start ()
{
	gettimeofday (&t0, NULL);
}

void accum ()
{
	gettimeofday (&t1, NULL);
	float fms = (t1.tv_sec - t0.tv_sec)*1e3 +
		   (t1.tv_usec - t0.tv_usec)*1e-3;
	int ms = (int) fms;
	if (ms > MAX_HISTO)
		ms = MAX_HISTO;
	h[ms]++;
}

int main (int c, char **v)
{
	int i, fd, r;
	if (c<5)
	{
		printf ("%s fwd|rev|chaotic read|write /device/name #512blocks\n", v[0]);
		return 1;
	}
	init ();
	char order=v[1][0];
	char dir=v[2][0];
	int count=atoi(v[4]);
	if (dir == 'r')
	{
		// Read tests
		fd = open (v[3], O_RDONLY | O_DSYNC);
		if (fd < 0)
		{
			perror("open for read");
			return 1;
		}
		if (order == 'c')
		{
			for (i=0; i<count; i++)
			{
				r = random() % count;
				start ();
				r = pread (fd, buf, BLOCK, r*BLOCK);
				accum ();
			}
		}
		else if (order == 'r')
		{
			for (i=count-1; i>=0; i--)
			{
				start ();
				r = pread (fd, buf, BLOCK, i*BLOCK);
				accum ();
			}
		}
		else
		{
			for (i=0; i<count; i++)
			{
				start ();
				r = pread (fd, buf, BLOCK, i*BLOCK);
				accum ();
			}
		}
		close (fd);
	}
	else
	{
		fd = open (v[3], O_WRONLY | O_DSYNC);
		if (fd < 0)
		{
			perror("open for write");
			return 1;
		}
		if (order == 'c')
		{
			for (i=0; i<count; i++)
			{
				r = random() % count;
				start ();
				r = pwrite (fd, buf, BLOCK, r*BLOCK);
				accum ();
			}
		}
		else if (order == 'r')
		{
			for (i=count-1; i>=0; i--)
			{
				start ();
				r = pwrite (fd, buf, BLOCK, i*BLOCK);
				accum ();
			}
		}
		else
		{
			for (i=0; i<count; i++)
			{
				start ();
				r = pwrite (fd, buf, BLOCK, i*BLOCK);
				accum ();
			}
		}
		close (fd);
	}
	printf ("dir=%c order=%c count=%d\n", dir, order, count);
	show (count);
	return 0;
}
