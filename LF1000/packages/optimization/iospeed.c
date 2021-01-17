#include <stdio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#define BIG_FILE	"/dev/mtd4"
#define SIZE	(128*1024)

main (int c, char **v)
{
	char big_file[128] = BIG_FILE;
	if (c>1)
		strcpy (big_file, v[1]);
	int fd = -1;
	struct timeval t0, t1;
	int total;
	char *buf = malloc (SIZE);
	if (!buf)
	{
		perror ("Can't malloc ");
		exit (1);
	}
	for (;;)
	{
		int len = read (fd, buf, SIZE); // Will fail first time
		if (len < SIZE)
		{
			if (fd>=0)
				close (fd);
			fd = open (big_file, O_RDONLY);
			if (!fd)
			{
				fprintf (stderr, "%s...", big_file);
				perror ("Can't open");
				exit (1);
			}
		}
		else
		{
			gettimeofday (&t1, NULL);
			total += len;
			if (t1.tv_sec == t0.tv_sec)
				continue;
			double f = (t1.tv_sec - t0.tv_sec)*1e6 +
				(t1.tv_usec - t0.tv_usec);
			f = total/f;
			printf ("IO=%.3f ", f);
			fflush (stdout);
		}
		total = 0;
		gettimeofday (&t0, NULL);
	}
}
