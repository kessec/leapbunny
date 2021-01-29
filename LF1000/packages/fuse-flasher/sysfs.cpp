#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "fuse-flasher.h"

int get_pipe_str (const char *pipe, char *response, int maxlen)
{
	const char *path = pipe; // Just for error reporting
	FILE *f = popen (pipe, "r");
	if (!f)
	{
		PERROR ("failed popen for read");
		return -EIO;
	}
	if (fgets (response, maxlen, f) == 0)
	{
		PERROR ("failed read on pipe");
		return -EIO;
	}
	int r = pclose (f);
	if (r != 0)
	{
		PERROR ("failed pclose");
		return -EIO;
	}
	// Chomp \n
	int len = strlen (response);
	if (len>1 && response[len-1]=='\n')
		response[--len]=0;
	return len;
}

int get_sysfs_str (const char *sysfs, char *response, int maxlen)
{
	int fd = open (sysfs, O_RDONLY);
	if (fd<0)
	{ 
		const char *path=sysfs;
		PERROR ("attemp to read");
		return -EIO;
	}
	int len = read (fd, response, maxlen);
	close (fd);
	// Chomp \n
	if (len>0)
		len = strlen (response);
	if (len>1 && response[len-1]=='\n')
		response[--len]=0;
	return len;
}

int set_sysfs_str (const char *sysfs, const char *value)
{
	int fd = open (sysfs, O_WRONLY);
	if (fd<0)
	{ 
		const char *path=sysfs;
		PERROR ("attemp to write");
		return -EIO;
	}
	int len = write (fd, value, strlen(value));
	close (fd);
	return len;
}

