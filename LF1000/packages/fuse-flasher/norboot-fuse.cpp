#include <fuse/fuse.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mount.h>

#include "sysfs.h"
#include "fuse-flasher.h"
#include "sd-helpers.h"
#include "sd-raw-fuse.h"
#include "filenames.h"

// Super hacko for returning getattr on created file
#define NAME_LEN 1024
// static char created[NAME_LEN] = {0}; 

int norboot_getattr(const char *path, struct stat *stbuf)
{
	// top directory should always be empty and be writeable only
	if(strcmp(path, NORBOOT_DIR) == 0)
	{
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 2;
		RETURN("/sd/raw: DIR|777");
		return 0;
	}
	if (seen_path (path))
	{
		stbuf->st_mode = S_IFREG | 0666;
		stbuf->st_nlink = 1;
		NOTE1("seen: %s FILE|666", path);
		return 0;
	}
	RETURN ("!!! error");
	return -ENOENT;
}

int norboot_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	if(strcmp(path, NORBOOT_DIR) == 0)
	{
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		return 0;
	}
	return -ENOENT;
}

int norboot_open(const char* path, struct fuse_file_info* fi)
{
	// We expect a path like this:
	//   /NORBoot/madrid-boot.bin
	// We actually just store the data and flash it when we flush
	NOTE1 ("flags=%o", fi->flags);
	if ((fi->flags & O_ACCMODE) == O_RDWR)
	{
		// DFTP's ipkg cmd does this little dance at the end of a file:
		//    open (O_RDRW); fchmod, fsync, close
		// so we need to respect it
		RETURN ("Early exit for O_RDWR");
		return 0;
	}

	// In a perfect world, we would create a new name like mktemp() and
	// save the data there.  But we only get fi->fh to store our data
	// and I'm lazy, so I'm just going to use the basename of the path
	// and put it in /tmp.

	// We know our prefix is NORBOOT_DIR, so there is a slash.
	// Do a 10c basename on the string
	char tmpname[NAME_LEN];
	char *slash = strrchr (path, '/');
	if (!slash || slash[1]==0)
	{
		RETURN ("Malformed path");
		return 1;
	}
	sprintf (tmpname, "/tmp/%s", slash+1);
	
	// Open for writing
	int fh = open (tmpname, O_WRONLY | O_CREAT);
	if (fh == -1)
	{
		NOTE1 ("tmpname='%s'", tmpname);
		PERROR ("failed open");
		return -EIO;
	}
	fi->fh = fh;
	RETURN ("Success");
	return 0;
}

int norboot_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
	//Placeholder in case we want to allow reading for verification purposes later
	return -ENOENT;
}

int norboot_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
	int r = write (fi->fh, buf, size); 
	NOTE2 ("%d bytes. Ret=%d", size, r);
	return r;
}

int norboot_flush(const char* path, struct fuse_file_info* fi)
{
	ENTER;

	NOTE1 ("fi->fh=%d", (int) fi->fh);
	if (fi->fh == 0)
	{
		// DFTP's ipkg cmd does this little dance at the end of a file:
		//    open (O_RDRW); fchmod, fsync, close
		// so we need to respect it
		RETURN ("Skip flush");
		return 0;
	}

	// Flush data so far
	if (fsync (fi->fh) == -1)
	{
		PERROR ("failed fsync");
		return -EIO;
	}
	close (fi->fh);
	
	// Find the NORBoot device
	char dev[NAME_LEN];
	int r = get_pipe_str ("fgrep 'NOR_Boot' /proc/mtd | cut -d : -f 1", dev, NAME_LEN);
	if (r < 0)
	{
		NOTE1 ("Dev=%s", dev);
		PERROR ("failed pipe for fetching NOR_Boot device name");
		return -EIO;
	}

	// Recompute our file name
	// We know our prefix is NORBOOT_DIR, so there is a slash.
	// Do a 10c basename on the string
	char tmpname[NAME_LEN];
	char *slash = strrchr (path, '/');
	if (!slash || slash[1]==0)
	{
		RETURN ("Malformed path");
		return 1;
	}
	sprintf (tmpname, "/tmp/%s", slash+1);

	// Now... pipe out of flashcp
	//   mfgmode.sh 9 > /dev/null
	//   flashcp -v $eb $DEV
	//   mfgmode.sh 0 > /dev/null
	r = system ("mfgmode.sh 9 > /dev/null");
	if (r)
	{
		PERROR ("Trouble with mfgmode.sh 9");
		return -EIO;
	}
	char cmd[NAME_LEN];
	sprintf (cmd, "flashcp -v \"%s\" /dev/%s", tmpname, dev);
	NOTE1 ("try '%s'", cmd);
	r = system (cmd);
	if (r)
	{
		NOTE2 ("try '%s' Return=%d", cmd, r);
		PERROR ("Trouble with cmd!");
		return -EIO;
	}
	r = system ("mfgmode.sh 0 > /dev/null");
	if (r)
	{
		PERROR ("Trouble with mfgmode.sh 0");
		return -EIO;
	}
	

	RETURN ("Success");
	return 0;
}

int norboot_create(const char* path, mode_t mode, 
			struct fuse_file_info* fi)
{
	add_path (path);
	NOTE1 ("mode=%o", mode);
	return norboot_open(path, fi);
}
