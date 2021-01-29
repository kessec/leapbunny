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

extern const char* SD_RAW_DIR;

// Super hacko for returning getattr on created file
#define NAME_LEN 1024
// static char created[NAME_LEN] = {0}; 

int sd_raw_getattr(const char *path, struct stat *stbuf)
{
	// raw directory should always be empty and be writeable only
	if(strcmp(path, SD_RAW_DIR) == 0)
	{
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 2;
		RETURN("/sd/raw: DIR|777");
		return 0;
	}
	if(strstr(path, SD_RAW_DIR) == path &&
	   path[strlen(SD_RAW_DIR)] == '/' &&
	   path[strlen(SD_RAW_DIR)+1] >= '1' &&
	   path[strlen(SD_RAW_DIR)+1] <= '4' &&
	   path[strlen(SD_RAW_DIR)+2] == 0)
	{
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 2;
		RETURN("/sd/raw/N: DIR|777");
		return 0;
	}
#if 0
	// Hacko: If we just created the file, respond that it is there
	if (strcmp(path, created) == 0)
	{
		stbuf->st_mode = S_IFREG | 0666;
		stbuf->st_nlink = 1;
		RETURN("/sd/raw/created: FILE|666");
		return 0;
	}
#else
	if (seen_path (path))
	{
		stbuf->st_mode = S_IFREG | 0666;
		stbuf->st_nlink = 1;
		NOTE1("seen: %s FILE|666", path);
		return 0;
	}
#endif
	RETURN ("!!! error");
	return -ENOENT;
}

int sd_raw_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	if(strcmp(path, SD_RAW_DIR) == 0)
	{
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		return 0;
	}
	return -ENOENT;
}

int sd_raw_open(const char* path, struct fuse_file_info* fi)
{
	// We expect a path like this:
	//   /sd/raw/N/who-cares-whats-here
	// where N in 1..4 corresponding to the partition number
	NOTE1 ("flags=%o", fi->flags);
	if ((fi->flags & O_ACCMODE) == O_RDWR)
	{
		// DFTP's ipkg cmd does this little dance at the end of a file:
		//    open (O_RDRW); fchmod, fsync, close
		// so we need to respect it
#if 0
		strcpy (created, path);
#endif
		RETURN ("Early exit for O_RDWR");
		return 0;
	}

	// Get the partition number
	int offset = strlen (SD_RAW_DIR);
	if (offset <= 0)
	{
		RETURN ("path is broken");
		return -EIO;
	}
	int pnum = atoi (path+offset+1); // skip /
	NOTE1 ("using mmblk #=%d'", pnum);

	// Get device name for partition of interest
	struct stat buf;
	char *mmcblk = get_mmcblk (NULL);
	if (mmcblk == NULL)
	{
		PERROR ("Can't get mmcblk name");
		return -EIO;
	}
	NOTE1 ("mmcblk='%s'", mmcblk);

	char devmmcblkXpN[NAME_LEN];
	sprintf (devmmcblkXpN, "/dev/%sp%d", mmcblk, pnum);
	if (stat (devmmcblkXpN, &buf) == -1)
	{
		NOTE1 ("devmmcblk='%s'", devmmcblkXpN);
		PERROR ("Can't find partition for creating raw");
		return -EIO;
	}

	// Open for writing
	int fh = open (devmmcblkXpN, O_WRONLY);
	if (fh == -1)
	{
		NOTE1 ("dev='%s'", devmmcblkXpN);
		PERROR ("failed open");
		return -EIO;
	}
	fi->fh = fh;
	RETURN ("Success");
	return 0;
}

int sd_raw_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
	//Placeholder in case we want to allow reading for verification purposes later
	return -ENOENT;
}

int sd_raw_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
	int r = write (fi->fh, buf, size); 
	NOTE2 ("%d bytes. Ret=%d", size, r);
	return r;
}

int sd_raw_flush(const char* path, struct fuse_file_info* fi)
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
	
	RETURN ("Success");
	return 0;
}

int sd_raw_create(const char* path, mode_t mode, 
			struct fuse_file_info* fi)
{
#if 0	
	strcpy (created, path);
#else
	add_path (path);
#endif
	NOTE1 ("mode=%o", mode);
	return sd_raw_open(path, fi);
}
