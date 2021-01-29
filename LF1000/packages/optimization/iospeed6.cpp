#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

// r|w device #512blocks

#define BLOCK	512
char buf[BLOCK];

int main (int c, char **v)
{
	int i, fd, r;
	if (c<5)
	{
		printf ("%s fwd|rev read|write /device/name #512blocks\n", v[0]);
		return 1;
	}
	if (v[2][0] == 'r')
	{
		fd = open (v[3], O_RDONLY | O_DSYNC);
		if (fd < 0)
		{
			perror("open for read");
			return 1;
		}
		if (v[1][0] == 'r')
		{
			for (i=atoi(v[4])-1; i>=0; i--)
				r = pread (fd, buf, BLOCK, i*BLOCK);
		}
		else
		{
			for (i=0; i<atoi(v[4]); i++)
				r = pread (fd, buf, BLOCK, i*BLOCK);
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
		if (v[1][0] == 'r')
		{
			for (i=atoi(v[4])-1; i>=0; i--)
				r = pwrite (fd, buf, BLOCK, i*BLOCK);
		}
		else
		{
			for (i=0; i<atoi(v[4]); i++)
				r = pwrite (fd, buf, BLOCK, i*BLOCK);
		}
		close (fd);
	}
	return 0;
}

