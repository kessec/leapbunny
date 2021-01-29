#include <fuse/fuse.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "sysfs.h"
#include "fuse-flasher.h"
#include "sd-helpers.h"
#include "filenames.h"

// Super hacko for returning getattr on created file
#define NAME_LEN 1024
// static char created[NAME_LEN] = {0}; 

int sd_partition_getattr(const char *path, struct stat *stbuf)
{
	//Partition directory should always be empty and be writeable only
	if(strcmp(path, SD_PARTITION_DIR) == 0)
	{
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 2;
		RETURN("/sd/partition: DIR|777");
		return 0;
	}
	// Hacko: If we just created a file, respond that it is there
	if (seen_path (path))
	{
		stbuf->st_mode = S_IFREG | 0666;
		stbuf->st_nlink = 1;
		NOTE1("seen: %s FILE|666", path);
		return 0;
	}
	RETURN ("error");
	return -ENOENT;
}

int sd_partition_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	if(strcmp(path, SD_PARTITION_DIR) == 0)
	{
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		RETURN ("/sd/partition: . and ..");
		return 0;
	}
	RETURN ("error");
	return -ENOENT;
}

int sd_partition_open(const char* path, struct fuse_file_info* fi)
{
	// We expect a path like this:
	//   /sd/partition/any-name-here
	// Must achieve these things before data starts flowing:
	//   Determine the device: typically /dev/mmcblk0
	//   Open it for writing
	NOTE1 ("flags=%o", fi->flags);
	if ((fi->flags & O_ACCMODE) == O_RDWR)
	{
		// DFTP's ipkg cmd does this little dance at the end of a file:
		//    open (O_RDRW); fchmod, fsync, close
		// so we need to respect it
		fi->fh = 0;
#if 0
		strcpy (created, path);
#endif
		RETURN ("Early exit for O_RDWR");
		return 0;
	}

	// Make sure there is a /dev/mmcblkN
	struct stat buf;
	char *mmcblk = get_mmcblk (NULL);
	if (mmcblk == NULL)
	{
		PERROR ("Can't get mmcblk name");
		return -EIO;
	}
	NOTE1 ("mmcblk='%s'", mmcblk);
	char devmmcblk[NAME_LEN];
	sprintf (devmmcblk, "/dev/%s", mmcblk);
	if (stat (devmmcblk, &buf) == -1)
	{
		PERROR ("Can't find mmcblk for writing partition info");
		return -EIO;
	}
	// Open for writing
	int fh = open (devmmcblk, O_WRONLY);
	if (fh == -1)
	{
		PERROR ("Can't open mmcblk for writing partition info");
		return -EIO;
	}
	fi->fh = fh;
	EXIT;
	return 0;
}

int sd_partition_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
	//Placeholder in case we want to allow reading for verification purposes later
	return -ENOENT;
}

int sd_partition_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
	int r = write (fi->fh, buf, size);
	NOTE2 ("%d bytes. Ret=%d", size, r);
	return r;
}

int sd_partition_flush(const char* path, struct fuse_file_info* fi)
{
	// Must achieve these things now that data is (hopefully) done
	//   Flush and Close the /dev/mmcblkN file
	//   Unbind and rebind the device to make partition take effect
	//   mdev -s to make sure all /dev nodes are up to date

	// Flush data so far
	ENTER;

	// Hacko: Forget that we created a file...
#if 0
	// created[0] = 0;
#endif

	NOTE1 ("fi->fh=%d", (int)fi->fh);
	if (fi->fh == 0)
	{
		// DFTP's ipkg cmd does this little dance at the end of a file:
		//    open (O_RDRW); fchmod, fsync, close
		// so we need to respect it
		RETURN ("Skip flush");
		return 0;
	}

	if (fsync (fi->fh) == -1)
	{
		PERROR ("failed fsync");
		return -EIO;
	}
	close (fi->fh);

	// Find device name to pass to bind/unbind
#define SYS_BUS_MMC_DEVICES "/sys/bus/mmc/devices"
	char *dev = get_mmcdev ();
	if (dev == NULL)
	{
		PERROR ("failed pipe for fetching " SYS_BUS_MMC_DEVICES);
		return -EIO;
	}
	NOTE1 ("found device=%s", dev);
	
	// Do unbind and bind
#define SYS_BUS_MMC_DRIVERS_MMCBLK_UNBIND "/sys/bus/mmc/drivers/mmcblk/unbind"
#define SYS_BUS_MMC_DRIVERS_MMCBLK_BIND "/sys/bus/mmc/drivers/mmcblk/bind"
	if (set_sysfs_str (SYS_BUS_MMC_DRIVERS_MMCBLK_UNBIND, dev)
	    < strlen(dev))
	{
		PERROR ("failed open for write on " SYS_BUS_MMC_DRIVERS_MMCBLK_UNBIND);
		return -EIO;
	}
	NOTE1 ("device=%s unbound", dev);
	if (set_sysfs_str (SYS_BUS_MMC_DRIVERS_MMCBLK_BIND, dev)
	    < strlen(dev))
	{
		PERROR ("failed open for write on " SYS_BUS_MMC_DRIVERS_MMCBLK_BIND);
		return -EIO;
	}
	NOTE1 ("device=%s bound", dev);

	// Find the actual device we bound back onto
	char *mmcblk = get_mmcblk (dev);
	if (!mmcblk)
	{
		PERROR ("failed finding mmcblk device");
		return -EIO;
	}
	NOTE1 ("got mmcblk='%s'\n", mmcblk);

	// Wake up missing mmcblk device
	system ("mdev -s");

	// Verify it made it: look for /dev/mmcblk0p4
	char verify[NAME_LEN];
	struct stat buf;
	sprintf (verify, "/dev/%sp1", mmcblk);
	if (stat (verify, &buf) == -1)
	{
		NOTE1 ("p1=%s", verify);
		PERROR ("Can't find p1 partition");
		return -EIO;
	}
	sprintf (verify, "/dev/%sp4", mmcblk);
	if (stat (verify, &buf) == -1)
	{
		NOTE1 ("p4=%s", verify);
		PERROR ("Can't find p4 partition");
		return -EIO;
	}
	EXIT;
	return 0;
}

int sd_partition_create(const char* path, mode_t mode, 
			struct fuse_file_info* fi)
{
#if 0	
	strcpy (created, path);
#else
	add_path (path);
#endif
	NOTE1 ("mode=%o", mode);
	return sd_partition_open(path, fi);
}
