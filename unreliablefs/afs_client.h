#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/statvfs.h>

#include <grpc++/grpc++.h>
#include "afs.grpc.pb.h"


using afs::AuthData;
using afs::AfsService;

class AfsClient {
    public:
        // Fuse interface
        // NOTE: do not add fuse dependency on this file!
        AfsClient(const char *serveraddr, const char *cachedir);
        int fuse_lstat(const char *path, struct stat *buf);
        int fuse_mkdir(const char *path, mode_t mode);
        int fuse_rmdir(const char *path);
        int fuse_rename(const char *oldpath, const char* newpath);
        int fuse_truncate(const char *path, off_t length);
        int fuse_open(const char* path, int flags);
        int fuse_creat(const char* path, int flags, mode_t mode);
        int fuse_statvfs(const char *path, struct statvfs *buf);
        DIR* fuse_opendir(const char *path);
        int fuse_close(int fd, const char *path, bool received_write);
        int fuse_unlink(const char *path);
        int fuse_access(const char *path, int amode);
        bool server_alive();

    private:
        // Wrapper functions around stub functions
        int Fetch(std::string const& remotepath);
        int Store(std::string const& remotepath);
        int Remove(std::string const& remotepath);
        int Create(std::string const& remotepath);
        int Rename(std::string const& oldpath, std::string const& newpath);
        int Makedir(std::string const& remotepath);
        int Removedir(std::string const& remotepath);
        AuthData TestAuth(std::string const& remotepath);

        std::string getAFSPath(std::string const& path);
        int makeParentDirs(std::string const& remotepath);
        int fetchRegular(std::string const& remotepath, AuthData& authdata);
        int fetchDirectory(std::string const& remotepath);
        DIR* fuseOpenDir(std::string const& remotepath);

        std::unique_ptr<AfsService::Stub> stub_;
        std::string cachedir_;
};
