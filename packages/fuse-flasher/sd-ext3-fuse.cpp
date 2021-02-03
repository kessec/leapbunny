#include <fuse/fuse.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <time.h>

#include "sysfs.h"
#include "fuse-flasher.h"
#include "sd-helpers.h"
#include "sd-ext3-fuse.h"
#include "filenames.h"

extern const char* SD_EXT_DIR;

// Super hacko for returning getattr on created file
#define NAME_LEN 1024
// static char created[NAME_LEN] = {0}; 

int sd_ext_getattr(const char *path, struct stat *stbuf)
{
	// ext3 and ext4 directory should always be empty and be writeable only
	if(strlen(path) == strlen(SD_EXT_DIR)+1)
	{
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 2;
		RETURN("/sd/ext#: DIR|777");
		return 0;
	}
	if(strstr(path, SD_EXT_DIR) == path &&
	   path[strlen(SD_EXT_DIR)+1] == '/' &&
	   path[strlen(SD_EXT_DIR)+2] >= '1' &&
	   path[strlen(SD_EXT_DIR)+2] <= '4' &&
	   path[strlen(SD_EXT_DIR)+3] == 0)
	{
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 2;
		RETURN("/sd/ext#/N: DIR|777");
		return 0;
	}
	// Hacko: If we just created the file, respond that it is there
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

int sd_ext_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	if(strlen(path) == strlen(SD_EXT_DIR) + 1)
	{
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		return 0;
	}
	return -ENOENT;
}

int sd_ext_open(const char* path, struct fuse_file_info* fi)
{
	ENTER;
	// Time how long this takes and report
	time_t t0, t1;
	time (&t0);
	// Must achieve these things before data starts flowing:
	//   mkfs.ext3 -L label /dev/mmcblkXpN
	//   mount /dev/mmcblkXpN /dev/mmcblkXpN
	// We expect a path like this:
	//   /sd/ext3/N/label
	// where N in 1..4 and label is the desired volume label
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
	if ((fi->flags & O_ACCMODE) == O_RDONLY)
		return -EACCES;

	// Get the partition number
	int offset = strlen (SD_EXT_DIR) + 1;
	if (offset <= 0)
	{
		RETURN ("path is broken");
		return -EIO;
	}
	int pnum = atoi (path+offset+1); // skip /
	const char *label = path+offset+3; // skip partition # and 2nd /  
	NOTE2 ("using mmblk #=%d Label='%s'", pnum, label);

	// Get device name for partition of interest
	struct stat buf;
	char *mmcblk = get_mmcblk (NULL);
	if (mmcblk == NULL)
	{
		PERROR ("Can't get mmcblk name");
		return -EIO;
	}

	char devmmcblkXpN[NAME_LEN];
	sprintf (devmmcblkXpN, "/dev/%sp%d", mmcblk, pnum);
	if (stat (devmmcblkXpN, &buf) == -1)
	{
		NOTE1 ("devmmcblk='%s'", devmmcblkXpN);
		PERROR ("Can't find partition for creating ext#");
		return -EIO;
	}

	// Mkfs
	char cmd[NAME_LEN];
	int ext_type = atoi(path + strlen(SD_EXT_DIR));
	sprintf (cmd, "mkfs.ext%d -L \"%s\" %s", ext_type, label, devmmcblkXpN);
	NOTE1 ("try '%s'", cmd);
	int r = system (cmd);
	if (r)
	{
		NOTE2 ("try '%s' Return=%d", cmd, r);
		PERROR ("Trouble with cmd!");
		return -EIO;
	}
	
	//Tune fs to default to writeback journal
	sprintf (cmd, "tune2fs -o journal_data_writeback %s", devmmcblkXpN);
	NOTE1 ("try '%s'", cmd);
	r = system (cmd);
	if (r)
	{
		NOTE2 ("try '%s' Return=%d", cmd, r);
		PERROR ("Trouble with cmd!");
		return -EIO;
	}

	// Mount
#define MOUNT_PT "/mnt"
	sprintf(cmd, "ext%d", ext_type);
	r = mount (devmmcblkXpN, MOUNT_PT, cmd, MS_NOATIME, "");
	if (r)
	{
		NOTE2 ("mount failed: dev=%s return=%d", devmmcblkXpN, r);
		PERROR ("Trouble with mount");
		return -EIO;
	}

	// Finally, open a pipe to tar
	sprintf (cmd, "tar -C %s -x", MOUNT_PT);
	NOTE1 ("%s", cmd);
	FILE *f = popen (cmd, "w");
	if (!f)
	{
		NOTE1 ("popen=%s ", cmd);
		PERROR ("could not open pipe to tar");
		return -EIO;
	}
	// Save the handle
	fi->fh = (uint64_t) f;
	NOTE1 ("Filehandle: %lu", fi->fh);
	// Report time
	time (&t1);
	NOTE1 ("Time=%d seconds", (int)(t1-t0));
	RETURN ("Success making ext3 and mounting");
	return 0;
}

int sd_ext_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
	//Placeholder in case we want to allow reading for verification purposes later
	return -ENOENT;
}

static int write_printf_counter=0;

int sd_ext_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
	FILE *f = (FILE *)fi->fh;
	int r=fwrite (buf, 1, size, f);
	if (!(write_printf_counter++ % 1000))
		NOTE2 ("%d bytes. Ret=%d", size, r);
	return r;
}

int sd_ext_flush(const char* path, struct fuse_file_info* fi)
{
	write_printf_counter = 0;

	// Flush data so far
	ENTER;

	// Hacko: Forget that we created a file...
#if 0
	created[0] = 0;
#endif

	NOTE1 ("fi->fh=%d", (int) fi->fh);
	if (fi->fh == 0)
	{
		// DFTP's ipkg cmd does this little dance at the end of a file:
		//    open (O_RDRW); fchmod, fsync, close
		// so we need to respect it
		RETURN ("Skip flush");
		return 0;
	}

	FILE *f = (FILE *)fi->fh;
	int r = pclose (f);
	if (r != 0)
	{
		NOTE1 ("failed pclose r=%d", r);
		PERROR ("failed pclose");
		return -EIO;
	}
	
	// Unmount
	if (umount (MOUNT_PT) == -1)
	{
		PERROR ("sd_ext_flush: failed umount MOUNT_PT");
		return -EIO;
	}
		
	EXIT;
	return 0;
}

int sd_ext_create(const char* path, mode_t mode, 
			struct fuse_file_info* fi)
{
#if 0	
	strcpy (created, path);
#else
	add_path (path);
#endif
	NOTE1 ("mode=%o", mode);
	return sd_ext_open(path, fi);
}
