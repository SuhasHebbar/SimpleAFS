#pragma once

#include <dirent.h>
#include <fcntl.h>
#include <fuse.h>
#include <sys/stat.h>
#include <sys/types.h>

int afs_fuse_setup(const char *serveraddr, const char *cachedir);
void afs_fuse_teardown();

int afs_fuse_lstat(const char *path, struct stat *buf);
int afs_fuse_mkdir(const char *path, mode_t mode);
int afs_fuse_rmdir(const char *path);
int afs_fuse_rename(const char *oldpath, const char *newpath);
int afs_fuse_truncate(const char *path, off_t length);
int afs_fuse_open(const char *path, int flags);
int afs_fuse_creat(const char *path, int flags, mode_t mode);
int afs_fuse_statvfs(const char *path, struct statvfs *buf);
DIR *afs_fuse_opendir(const char *path);
int afs_fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi);
int afs_fuse_closedir(DIR* dp);
int afs_fuse_close(int fd, const char *path);
int afs_fuse_unlink(const char *path);
int afs_fuse_access(const char *path, int amode);
