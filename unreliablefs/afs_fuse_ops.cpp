#include <cerrno>
#include <memory>

#include "afs_client.h"

namespace {

std::unique_ptr<AfsClient> g_afsClient;

} // anonymous namespace

extern "C" {

int afs_fuse_setup(const char *serveraddr, const char *cachedir) {
    if (mkdir("/tmp/afs", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0 && errno != EEXIST) {
   	return -1;
    }

    if (mkdir("/tmp/afs/client", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0 && errno != EEXIST) {
   	return -1;
    }

    g_afsClient = std::make_unique<AfsClient>(serveraddr, cachedir);
    return 0;
}

void afs_fuse_teardown() {
    g_afsClient.reset();
}

int afs_fuse_lstat(const char *path, struct stat *buf) {
    return g_afsClient->fuse_lstat(path, buf);
}

int afs_fuse_mkdir(const char *path, mode_t mode) {
    return g_afsClient->fuse_mkdir(path, mode);
}

int afs_fuse_rmdir(const char *path) {
    return g_afsClient->fuse_rmdir(path);
}

int afs_fuse_rename(const char *oldpath, const char* newpath) {
    return g_afsClient->fuse_rename(oldpath, newpath);
}

int afs_fuse_truncate(const char *path, off_t length) {
	std::cout << __FILE__ << " " << __LINE__ << std::endl;
    int ret = g_afsClient->fuse_truncate(path, length);
	std::cout << __FILE__ << " " << __LINE__ << std::endl;
    return ret;
}

int afs_fuse_open(const char* path, int flags) {
    return g_afsClient->fuse_open(path, flags);
}

int afs_fuse_creat(const char* path, int flags, mode_t mode) {
    return g_afsClient->fuse_creat(path, flags, mode);
}

int afs_fuse_statvfs(const char *path, struct statvfs *buf) {
    return g_afsClient->fuse_statvfs(path, buf);
}

DIR* afs_fuse_opendir(const char *path) {
    return g_afsClient->fuse_opendir(path);
}

int afs_fuse_close(int fd, const char *path) {
    return g_afsClient->fuse_close(fd, path);
}

int afs_fuse_unlink(const char *path) {
    return g_afsClient->fuse_unlink(path);
}

int afs_fuse_access(const char *path, int amode) {
    return g_afsClient->fuse_access(path, amode);
}

} // extern "C"
