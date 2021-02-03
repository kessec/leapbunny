#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#define SIZE	(8*128*1024)
#define OVERSIZE (32*2)

#include "Stats.cpp"

main (int c, char **v)
{
	char big_file[128] = "file1.ios";
	if (c>1)
		strcpy (big_file, v[1]);

	int fd = -1;
	struct timeval t0, t1;
	char *buf = (char *)malloc (SIZE);
	if (!buf)
	{
		perror ("Can't malloc ");
		exit (1);
	}

	int i, total;
	double f;
	int writing=1;

	if (writing)
	{
		CStats s;
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
		for (i=0; i<OVERSIZE; i++)
		{
			gettimeofday (&t0, NULL);
			total = 0;
			int len = write (fd, buf, SIZE);
			if (len < SIZE)
			{
				fprintf (stderr, "trouble i=%d\n", i);
				perror ("Can't write");
				exit (1);
			}
			total += len;
			fdatasync (fd);
			gettimeofday (&t1, NULL);
			f = (t1.tv_sec - t0.tv_sec)*1e6 +
				(t1.tv_usec - t0.tv_usec);
			f = f/total; // Can't average MB/s, but can s/MB
			s.Add(f);
		}
		close (fd);
		printf ("Write N=%.0f %.3f/%.3f/%.3f MB/s\n", 
			s.N(), 1/s.Max(), 1/s.Mean(), 1/s.Min());
		fflush (stdout);
	}

	int reading=1;

	if (reading)
	{
		CStats s;
		// Try to read it back
		fd = open (big_file, O_RDONLY);
		if (!fd)
		{
			fprintf (stderr, "%s...", big_file);
			perror ("Can't open");
			exit (1);
		}
		for (i=0; i<OVERSIZE; i++)
		{
			fflush (stdout);
			gettimeofday (&t0, NULL);
			total = 0;
			int len = read (fd, buf, SIZE);
			if (len < SIZE)
			{
				fprintf (stderr, "trouble i=%d\n", i);
				perror ("Can't write");
				exit (1);
			}
			total += len;
			gettimeofday (&t1, NULL);
			f = (t1.tv_sec - t0.tv_sec)*1e6 +
				(t1.tv_usec - t0.tv_usec);
			f = f/total; // Can't average MB/s, but can s/MB
			s.Add(f);
		}
		close (fd);
		printf ("Read  N=%.0f %.3f/%.3f/%.3f MB/s\n", 
			s.N(), 1/s.Max(), 1/s.Mean(), 1/s.Min());
		fflush (stdout);
	}
}
