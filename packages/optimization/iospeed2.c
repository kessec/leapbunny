#include <stdio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#define SIZE	(128*1024)
#define OVERSIZE (8*32*2)


main (int c, char **v)
{
	char big_file[128] = "file1.ios";
	if (c>1)
		strcpy (big_file, v[1]);

	int fd = -1;
	struct timeval t0, t1;
	char *buf = malloc (SIZE);
	if (!buf)
	{
		perror ("Can't malloc ");
		exit (1);
	}

	int i, total;
	double f;
#if 1
	// Create some data
	for (i=0; i<SIZE; i++)
	{
		buf[i] = rand();
	}

	// Write out to file1
	unlink (big_file);
	fd = open (big_file, O_RDWR | O_CREAT, 0644);
	if (!fd)
	{
		fprintf (stderr, "%s...", big_file);
		perror ("Can't open");
		exit (1);
	}
	sync ();
	gettimeofday (&t0, NULL);
	total = 0;
	for (i=0; i<OVERSIZE; i++)
	{
		fflush (stdout);
		int len = write (fd, buf, SIZE);
		if (len < SIZE)
		{
			fprintf (stderr, "trouble i=%d\n", i);
			perror ("Can't write");
			exit (1);
		}
		total += len;
	}
	fdatasync (fd);
	close (fd);
	gettimeofday (&t1, NULL);
	f = (t1.tv_sec - t0.tv_sec)*1e6 +
		(t1.tv_usec - t0.tv_usec);
	f = total/f;
	printf ("Write =%.3f MB/s\n", f);
	fflush (stdout);

#endif
	// Try to read it back
	fd = open (big_file, O_RDONLY);
	if (!fd)
	{
		fprintf (stderr, "%s...", big_file);
		perror ("Can't open");
		exit (1);
	}
	gettimeofday (&t0, NULL);
	total = 0;
	for (i=0; i<OVERSIZE; i++)
	{
		fflush (stdout);
		int len = read (fd, buf, SIZE);
		if (len < SIZE)
		{
			fprintf (stderr, "trouble i=%d\n", i);
			perror ("Can't write");
			exit (1);
		}
		total += len;
	}
	close (fd);
	gettimeofday (&t1, NULL);
	f = (t1.tv_sec - t0.tv_sec)*1e6 +
		(t1.tv_usec - t0.tv_usec);
	f = total/f;
	printf ("Read  =%.3f MB/s\n", f);
	fflush (stdout);
}
