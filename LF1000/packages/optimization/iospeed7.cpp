#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <AtomicFile.h>
#include "Stats.cpp"

#define RANGE 2000
#define SCALE 10

main (int c, char **v)
{
	struct timeval t0, t1;
	int i;
	char FILE1[1024];
	int h[RANGE+1];
	int j=0, k, l;
	memset (h, 0, sizeof(h));
	for (l=0; l<20; l++)
	{
		CStats s;
		fclose (stderr);
		for (i=0; i<100; i++)
		{
			gettimeofday (&t0, NULL);
			sprintf (FILE1, "iospeed7-%d.dat", j++);
			FILE *f = fopenAtomic (FILE1, "wt");
			if (!f)
			{
				perror ("Can't open for write\n");
				exit (1);
			}
			for (k=0; k<1; k++)
				fwrite (FILE1, 1024, 1, f);
			// fprintf (f, "this is my test, line 1\n");
			int res = fcloseAtomic (f);
			if (res)
			{
				perror ("Failed fcloseAtomic\n");
				exit (1);
			}
			gettimeofday (&t1, NULL);
			double d = (t1.tv_sec - t0.tv_sec)*1e3 +
				(t1.tv_usec - t0.tv_usec)*1e-3;
			int b = (int)(d/SCALE);
			if (b>=RANGE)
				b=RANGE;
			h[b]++;
		}
		for (i=1; i<RANGE; i++)
		{
			if (h[i])
				printf ("%4d-%4d: %4d\n", (i-1)*SCALE, i*SCALE-1, h[i]);
		}
		printf ("Overflow: %4d\n", h[RANGE]);
	}
}
