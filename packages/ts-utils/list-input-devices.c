#include <stdio.h>
// #include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <linux/input.h>

// Find an input device by name and open it and return an fd or -1 for failure
main ()
{
	struct dirent *dp;
	char dev[20];
	char name[32];
	DIR *dir;
	int fd, i;
	FILE *f;

	dir = opendir("/dev/input/");
	if (!dir)
		return -1;
	f=popen("sort", "w");
	if (!f)
		return -1;
	while ((dp = readdir(dir)) != NULL) {
		if (dp->d_name && !strncmp(dp->d_name, "event", 5)) {
			sprintf(dev, "/dev/input/%s", dp->d_name);
			fd = open(dev, O_RDONLY);
			if (fd == -1)
				continue;
			if (ioctl(fd, EVIOCGNAME(32), name) < 0) {
				close(fd);
				continue;
			}
			fprintf (f, "%s: %s\n", dev, name);
			close(fd);
		}
	}
	closedir(dir);
	pclose (f);
	return 0;
}
