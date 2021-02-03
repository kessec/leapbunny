#ifndef _SD_RAW_FUSE_H_
#define _SD_RAW_FUSE_H_

int sd_raw_getattr(const char *path, struct stat *stbuf);
int sd_raw_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
int sd_raw_open(const char* path, struct fuse_file_info* fi);
int sd_raw_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi);
int sd_raw_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi);
int sd_raw_flush(const char* path, struct fuse_file_info* fi);
int sd_raw_create(const char* path, mode_t mode, struct fuse_file_info* fi);

#endif
