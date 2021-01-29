#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#define SIZE	(2048)
#define OVERSIZE (512)

#include "Stats.cpp"
#include <list>

// List of directories, files (paths)
// List that is log of what was done
// Chunk of data in a file that is 1/2 binary, 1/2 source.  Pick a random
//   2K section from 10K of file
// 0.1: Make directory and add to list
// 0.5: Make a file in some directory
// 0.4: Remove a file; remove parent directory if empty

// Test many small reads and writes
int main (int c, char **v)
{
	int i, fd, total, do_flush=0, do_write=1;
	if (c>1)
		do_write = atoi (v[1]);
	if (c>2) 
		do_flush = atoi (v[2]);
	printf ("do_write=%d do_flush=%d\n", do_write, do_flush);
	
	struct timeval t0, t1, t2;

	char here[4096];
	if (!getcwd (here, 4096))
		return 1;


	char big_file[128];
	char *buf = (char *)malloc (SIZE);
	if (!buf)
	{
		perror ("Can't malloc ");
		exit (1);
	}
	// Create some data
	for (i=0; i<SIZE; i++)
	{
		buf[i] = rand();
	}

	double f;

	if (do_write)
	{
		CStats s;

		// Write out many (OVERSIZE) files
		for (i=0; i<OVERSIZE; i++)
		{
			sprintf (big_file, "file%d.ios", i);
			unlink (big_file);
		}
		sync();
		gettimeofday (&t0, NULL);
		total = 0;
		for (i=0; i<OVERSIZE; i++)
		{
			gettimeofday (&t1, NULL);
			sprintf (big_file, "file%d.ios", i);
			fd = open (big_file, O_RDWR | O_CREAT, 0644);
			if (!fd)
			{
				fprintf (stderr, "%s...", big_file);
				perror ("Can't open");
				exit (1);
			}
			int len = write (fd, buf, SIZE);
			if (len < SIZE)
			{
				fprintf (stderr, "trouble i=%d\n", i);
				perror ("Can't write");
				exit (1);
			}
			total += len;
			if (do_flush)
				fdatasync (fd);
			close (fd);
			gettimeofday (&t2, NULL);
			f = (t2.tv_sec - t1.tv_sec)*1e6 +
				(t2.tv_usec - t1.tv_usec);
			f = f/len; // Can't average MB/s, but can s/MB
			s.Add(f);
		}
		f = (t2.tv_sec - t0.tv_sec)*1e6 +
			(t2.tv_usec - t0.tv_usec);
		f = f/total;
		printf ("Write N=%.0f %.2f/%.2f/%.2f Overall %.2f MB/s\n",
			s.N(), 1/s.Max(), 1/s.Mean(), 1/s.Min(), 1/f);
		fflush (stdout);
	}

	int do_read=1;
	if (do_read)
	{
		CStats s;

		// Read them back
		sync();
		gettimeofday (&t0, NULL);
		total = 0;
		for (i=0; i<OVERSIZE; i++)
		{
			gettimeofday (&t1, NULL);
			sprintf (big_file, "file%d.ios", i);
			fd = open (big_file, O_RDONLY);
			if (!fd)
			{
				fprintf (stderr, "%s...", big_file);
				perror ("Can't open");
				exit (1);
			}
			int len = read (fd, buf, SIZE);
			if (len < SIZE)
			{
				fprintf (stderr, "trouble i=%d\n", i);
				perror ("Can't read");
				exit (1);
			}
			total += len;
			gettimeofday (&t2, NULL);
			f = (t2.tv_sec - t1.tv_sec)*1e6 +
				(t2.tv_usec - t1.tv_usec);
			f = f/len;
			s.Add(f);
			close (fd);
		}
		f = (t2.tv_sec - t0.tv_sec)*1e6 +
			(t2.tv_usec - t0.tv_usec);
		f = f/total;
		printf ("Read  N=%.0f %.2f/%.2f/%.2f Overall=%.2f MB/s\n", 
			s.N(), 1/s.Max(), 1/s.Mean(), 1/s.Min(), 1/f);
		fflush (stdout);
	}
}
