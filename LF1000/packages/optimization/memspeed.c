#include <stdio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#define SIZE	(2*1024*1024)
#define J	5
#define JM	25

void mymemcpy (char *d, const char *s, int len)
{
	while (--len)
		*d++ = *s++;
}

void mymemcpy16 (short *d, const short *s, int len)
{
	len >>= 1;
	while (--len)
		*d++ = *s++;
}

void mymemcpy32 (long *d, const long *s, int len)
{
	len >>= 2;
	while (--len)
		*d++ = *s++;
}

main (int c, char **v)
{
	char buf0[SIZE], buf1[SIZE];
	struct timeval t0, t1;
	int i,j;
	double f;
	for (i=0; i<3; i++)
	{
		gettimeofday (&t0, NULL);
		for (j=0; j<J; j++)
			memcpy (buf1, buf0, SIZE);
		gettimeofday (&t1, NULL);
		f = J * SIZE / ((t1.tv_sec - t0.tv_sec) +
				(t1.tv_usec - t0.tv_usec) * 1e-6);
		printf ("memcpy = %.3f MB/s\n", f/1e6);

		gettimeofday (&t0, NULL);
		for (j=0; j<J; j++)
			mymemcpy (buf1, buf0, SIZE);
		gettimeofday (&t1, NULL);
		f = J * SIZE / ((t1.tv_sec - t0.tv_sec) +
				(t1.tv_usec - t0.tv_usec) * 1e-6);
		printf ("mymemcpy = %.3f MB/s\n", f/1e6);

		gettimeofday (&t0, NULL);
		for (j=0; j<J; j++)
			mymemcpy16 ((short *)buf1, (short *)buf0, SIZE);
		gettimeofday (&t1, NULL);
		f = J * SIZE / ((t1.tv_sec - t0.tv_sec) +
				(t1.tv_usec - t0.tv_usec) * 1e-6);
		printf ("mymemcpy16 = %.3f MB/s\n", f/1e6);

		gettimeofday (&t0, NULL);
		for (j=0; j<J; j++)
			mymemcpy32 ((long *)buf1, (long *)buf0, SIZE);
		gettimeofday (&t1, NULL);
		f = J * SIZE / ((t1.tv_sec - t0.tv_sec) +
				(t1.tv_usec - t0.tv_usec) * 1e-6);
		printf ("mymemcpy32 = %.3f MB/s\n", f/1e6);

		gettimeofday (&t0, NULL);
		for (j=0; j<J; j++)
			memset (buf1, j, SIZE);
		gettimeofday (&t1, NULL);
		f = J * SIZE / ((t1.tv_sec - t0.tv_sec) +
				(t1.tv_usec - t0.tv_usec) * 1e-6);
		printf ("memset = %.3f MB/s\n", f/1e6);

		gettimeofday (&t0, NULL);
		for (j=0; j<JM; j++)
			bzero (buf1, SIZE);
		gettimeofday (&t1, NULL);
		f = JM * SIZE / ((t1.tv_sec - t0.tv_sec) +
				(t1.tv_usec - t0.tv_usec) * 1e-6);
		printf ("bzero = %.3f MB/s\n", f/1e6);

		printf ("\n");
		fflush (stdout);
		sleep (1);
	}
}
