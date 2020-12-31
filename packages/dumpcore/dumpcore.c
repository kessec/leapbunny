/* 
Simple program to capture coredump through pipe
reference:  http://lwn.net/Articles/280959/
*/

#define _GNU_SOURCE
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h> 

#define BUF_SIZE 4096

// extern int errno;

int
main(int argc, char *argv[])
{
	int tot;
	ssize_t nread;
	char buf[BUF_SIZE];
	int fd;
	char fname[PATH_MAX];

	/* first parameter is the "flight recorder directory */
	if(argc > 1)
		chdir(argv[1]);

	/* second parameter is the program name that generated coredump */
	if(argc > 2) {
		snprintf(fname, PATH_MAX, "core_%s", argv[2]);
	} else {
		snprintf(fname, PATH_MAX, "core");
	}

	fd = open(fname, O_CREAT | O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "fail to open file to write, errno=%d\n", errno);
		exit(EXIT_FAILURE);
	}

	tot = 0;
	while ((nread = read(STDIN_FILENO, buf, BUF_SIZE)) > 0) {
		tot += nread;
		// fprintf(stdout, "Bytes read %d\n", nread);

		if(write(fd, buf, nread) == -1) {
		fprintf(stderr, "fail to generate core dump, due to errno %d\n", errno);
		close(fd);
		exit(EXIT_FAILURE);
		}
	}

	fprintf(stdout, "%s size %d\n", fname, tot);

	close(fd);
	exit(EXIT_SUCCESS);
   
}
