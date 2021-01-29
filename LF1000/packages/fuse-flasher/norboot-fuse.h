#ifndef _NORBOOT_FUSE_H_
#define _NORBOOT_FUSE_H_

int norboot_getattr(const char *path, struct stat *stbuf);
int norboot_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
int norboot_open(const char* path, struct fuse_file_info* fi);
int norboot_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi);
int norboot_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi);
int norboot_flush(const char* path, struct fuse_file_info* fi);
int norboot_create(const char* path, mode_t mode, struct fuse_file_info* fi);

#endif
