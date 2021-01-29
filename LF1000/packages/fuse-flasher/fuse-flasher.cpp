#include <stdio.h>
#include <string.h>	//for memset
#include <errno.h>

#define FUSE_USE_VERSION 27

#include <fuse/fuse.h>

#include "fuse-flasher.h"
#include "sd-partition-fuse.h"
#include "sd-ext3-fuse.h"
#include "sd-raw-fuse.h"
#include "norboot-fuse.h"
#include "filenames.h"

const char* FIRMWARE_DIR = "/firmware";
const char* SD_DIR = "/firmware/sd";
const char* SD_PARTITION_DIR = "/firmware/sd/partition";
const char* SD_EXT_DIR = "/firmware/sd/ext";
const char* SD_RAW_DIR = "/firmware/sd/raw";
const char* NORBOOT_DIR = "/firmware/NORBoot";

enum DirectoryType {
	IS_UNKOWN,
	IS_ROOTDIR,
	IS_FIRMWARE,
	IS_FIRMWAREDIR,
	IS_SD,
	IS_SD_PARTITION,
	IS_SD_EXT,
	IS_SD_RAW,
	IS_NORBOOT,
};

FILE *debug_out = NULL;
time_t debug_t0;

void init (void)
{
	if (!debug_out)
	{
		debug_out = fopen ("/dev/console", "w");
		debug_t0 = time(NULL);
	}
}

static DirectoryType check_path(const char* path)
{
	// The two directories that must match literally
	if(strcmp(path, "/") == 0)
		return IS_ROOTDIR;
	if(strcmp(path, FIRMWARE_DIR) ==0)
		return IS_FIRMWAREDIR;

	// Test for prefix of directory
	if(strstr(path, SD_PARTITION_DIR) == path)
		return IS_SD_PARTITION;
	if(strstr(path, SD_EXT_DIR) == path)
		return IS_SD_EXT;
	if(strstr(path, SD_RAW_DIR) == path)
		return IS_SD_RAW;
	if(strstr(path, NORBOOT_DIR) == path)
		return IS_NORBOOT;

	// Other prefix of directorie, but must come later since these
	// are substrings of above
	if(strstr(path, SD_DIR) == path)
		return IS_SD;
	if(strstr(path, FIRMWARE_DIR) == path)
		return IS_FIRMWARE;

	return IS_UNKOWN;
}

static int fuse_flasher_getattr(const char *path, struct stat *stbuf)
{	
	memset(stbuf, 0, sizeof(struct stat));
	
	DirectoryType dir_type = check_path(path);
	//Root directory is not writeable
	switch (dir_type)
	{
	case IS_FIRMWAREDIR:
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 2;
		RETURN ("DIR|777");
		return 0;
	case IS_ROOTDIR:
	case IS_SD:
		stbuf->st_mode = S_IFDIR | 0555;
		stbuf->st_nlink = 2;
		RETURN ("DIR|555");
		return 0;
	case IS_SD_PARTITION:
		return sd_partition_getattr(path, stbuf);
	case IS_SD_EXT:
		return sd_ext_getattr(path, stbuf);
	case IS_SD_RAW:
		return sd_raw_getattr(path, stbuf);
	case IS_NORBOOT:
		return norboot_getattr(path, stbuf);
	}
	// Uh, assume we're in rootdir and saving meta.inf... Ug
	if (seen_path (path))
	{
		stbuf->st_mode = S_IFREG | 0666;
		stbuf->st_nlink = 1;
		RETURN("FILE|666");
		return 0;
	}
	RETURN ("!!! error");
	return -ENOENT;
}

static int fuse_flasher_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi)
{
	DirectoryType dir_type = check_path(path);
	
	// Enter ();
	switch (dir_type)
	{
	case IS_ROOTDIR:
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		filler(buf, "firmware", NULL, 0);
		return 0;
	case IS_FIRMWAREDIR:
	case IS_FIRMWARE:
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		filler(buf, "sd", NULL, 0);
		filler(buf, "NORBoot", NULL, 0);
		return 0;
	case IS_SD:
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		filler(buf, "partition", NULL, 0);
		filler(buf, "raw", NULL, 0);
		filler(buf, "ext3", NULL, 0);
		filler(buf, "ext4", NULL, 0);
		return 0;
	case IS_SD_PARTITION:
	case IS_SD_EXT:
	case IS_SD_RAW:
	case IS_NORBOOT:
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		return 0;
	}
	RETURN ("!!! error");
	return -ENOENT;
}

static int fuse_flasher_open(const char* path, struct fuse_file_info* fi)
{
	DirectoryType dir_type = check_path(path);
	
	//Pass off system and firmware based calls to respective handlers
	if(dir_type == IS_SD_PARTITION)
		return sd_partition_open(path, fi);
	if(dir_type == IS_SD_EXT)
		return sd_ext_open(path, fi);
	if(dir_type == IS_SD_RAW)
		return sd_raw_open(path, fi);
	if(dir_type == IS_NORBOOT)
		return norboot_open(path, fi);
	// Uh, assume we're in rootdir and saving meta.inf... Ug
	if (seen_path (path))
	{
		RETURN("seen, skippping");
		return 0;
	}
	
	RETURN ("!!! error");
	return -ENOENT;
}

static int fuse_flasher_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
	DirectoryType dir_type = check_path(path);
	
	//Pass off system and firmware based calls to respective handlers
	if(dir_type == IS_SD_PARTITION)
		return sd_partition_read(path, buf, size, offset, fi);
	if(dir_type == IS_SD_EXT)
		return sd_ext_read(path, buf, size, offset, fi);
	if(dir_type == IS_SD_RAW)
		return sd_raw_read(path, buf, size, offset, fi);
	if(dir_type == IS_NORBOOT)
		return norboot_read(path, buf, size, offset, fi);
	
	RETURN ("!!! error");
	return -ENOENT;
}

static int fuse_flasher_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
	DirectoryType dir_type = check_path(path);
	
	//Pass off system and firmware based calls to respective handlers
	if(dir_type == IS_SD_PARTITION)
		return sd_partition_write(path, buf, size, offset, fi);
	if(dir_type == IS_SD_EXT)
		return sd_ext_write(path, buf, size, offset, fi);
	if(dir_type == IS_SD_RAW)
		return sd_raw_write(path, buf, size, offset, fi);
	if(dir_type == IS_NORBOOT)
		return norboot_write(path, buf, size, offset, fi);
	// Uh, assume we're in rootdir and saving meta.inf... Ug
	RETURN ("assume we're in rootdir and saving meta.inf... Ug");
	return size;
	// RETURN ("!!! error");
	// return -ENOENT;
}

static int fuse_flasher_flush(const char* path, struct fuse_file_info* fi)
{
	DirectoryType dir_type = check_path(path);
	
	//Pass off system based calls to respective handler
	if(dir_type == IS_SD_PARTITION)
		return sd_partition_flush(path, fi);
	if(dir_type == IS_SD_EXT)
		return sd_ext_flush(path, fi);
	if(dir_type == IS_SD_RAW)
		return sd_raw_flush(path, fi);
	if(dir_type == IS_NORBOOT)
		return norboot_flush(path, fi);
	// Uh, assume we're in rootdir and saving meta.inf... Ug
	RETURN ("assume we're in rootdir and saving meta.inf... Ug");
	return 0;
}

static int fuse_flasher_create(const char* path, mode_t mode, 
			   struct fuse_file_info* fi)
{
	DirectoryType dir_type = check_path(path);
	// Note1 ("dir_type=%d", dir_type);

	//Pass off system based calls to respective handler
	if(dir_type == IS_SD_PARTITION)
		return sd_partition_create (path, mode, fi);
	if(dir_type == IS_SD_EXT)
		return sd_ext_create (path, mode, fi);
	if(dir_type == IS_SD_RAW)
		return sd_raw_create (path, mode, fi);
	if(dir_type == IS_NORBOOT)
		return norboot_create (path, mode, fi);
	// Just accept the path
	add_path (path);
	RETURN ("adding path");
	return 0;
}	

static int fuse_flasher_chmod(const char* path, mode_t mode)
{
	EXIT;
	return 0;
}

static int fuse_flasher_chown(const char* path, uid_t u, gid_t g)
{
	EXIT;
	return 0;
}

static int fuse_flasher_truncate(const char* path, off_t size)
{
	EXIT;
	return 0;
}

static int fuse_flasher_unlink(const char* path)
{
	int retval;
	retval = remove_path(path);
	RETURN( "removing path" );
	if( retval )
		return 0;
	return -ENOENT;
}

static struct fuse_operations* fuse_flasher_operations;

int main(int argc, char *argv[])
{
	fuse_flasher_operations = new fuse_operations;
	fuse_flasher_operations->getattr= fuse_flasher_getattr;
	fuse_flasher_operations->chmod	= fuse_flasher_chmod;
	fuse_flasher_operations->chown	= fuse_flasher_chown;
	fuse_flasher_operations->readdir= fuse_flasher_readdir;
	fuse_flasher_operations->open	= fuse_flasher_open;
	fuse_flasher_operations->read	= fuse_flasher_read;
	fuse_flasher_operations->write	= fuse_flasher_write;
	fuse_flasher_operations->flush	= fuse_flasher_flush;
	fuse_flasher_operations->create	= fuse_flasher_create;
	fuse_flasher_operations->truncate=fuse_flasher_truncate;
	fuse_flasher_operations->unlink = fuse_flasher_unlink;
	
	// sd_partition_init();
	return fuse_main(argc, argv, fuse_flasher_operations, NULL);
}
