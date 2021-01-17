#include <stdio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#define SIZE	1000000

main ()
{
	int fd = -1;
	struct timeval t0, t1;
	int total;
	for (;;)
	{
		int i;
		float x = t0.tv_sec;
		for (i=0; i<SIZE; i++)
			x += i;
		gettimeofday (&t1, NULL);
		total += SIZE;
		if (t1.tv_sec == t0.tv_sec)
			continue;
		double f = (t1.tv_sec - t0.tv_sec) +
			(t1.tv_usec - t0.tv_usec)*1e-6;
		f = total/f;
		printf ("IO=%.3f ", f*1e-6);
		fflush (stdout);
		total = 0;
		gettimeofday (&t0, NULL);
	}
}
