#ifndef _SD_EXT_FUSE_H_
#define _SD_EXT_FUSE_H_

int sd_ext_getattr(const char *path, struct stat *stbuf);
int sd_ext_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
int sd_ext_open(const char* path, struct fuse_file_info* fi);
int sd_ext_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi);
int sd_ext_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi);
int sd_ext_flush(const char* path, struct fuse_file_info* fi);
int sd_ext_create(const char* path, mode_t mode, struct fuse_file_info* fi);

#endif
