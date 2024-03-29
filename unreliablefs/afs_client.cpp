#include "afs.grpc.pb.h"
#include "afs.pb.h"
#include "common.h"
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <grpc++/grpc++.h>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "afs_client.h"

using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientWriter;
using grpc::Status;

using afs::AfsService;
using afs::AuthData;
using afs::FileData;
using afs::IOResult;
using afs::Path;
using afs::RenameArgs;
using afs::StatData;
using afs::StoreData;
using afs::StoreResult;

static const int BLOCK_SIZE = 1 << 13;
static char iobuf[BLOCK_SIZE];

// To convert __LINE__ to a string literal
#define STR(x) #x
#define EXPAND(x) STR(x)

// Disable debug logs in release builds
#ifdef NDEBUG
#define D(x)
#else
#define D(x) x
#endif

namespace {

std::unique_ptr<AfsService::Stub> makeStub(const char *serveraddr) {
  return AfsService::NewStub(grpc::CreateChannel(
      std::string(serveraddr), grpc::InsecureChannelCredentials()));
}

std::string concatenatedPaths(std::string const &dir, std::string const &path) {
  if (dir.size() == 0 || path.size() == 0) {
    // Not correct put let's keep the client running.
    return path;
  }

  std::string cleanPath;
  if (path[0] == '/') {
    cleanPath = path.substr(1, std::string::npos);
  } else {
    cleanPath = path;
  }

  // The project seems to have a mix of things. So we check to make sure.
  if (dir[dir.size() - 1] == '/') {
    return dir + cleanPath;
  }

  return dir + "/" + cleanPath;
}

// removes all redundant slashes
std::string sanitize(std::string const &path) {
  int n = path.size();
  if (n == 0) {
    return "/";
  }
  std::string result{path[0]};
  for (auto i = 1; i < n; i++) {
    if (result.back() != '/' || path[i] != '/') {
      result += path[i];
    }
  }
  if (result.size() > 1 && result.back() == '/') {
    // Remove trailing slash
    result.pop_back();
  }
  return result;
}

std::vector<std::string> getAncestry(std::string const &remotepath) {
  std::string ldir;
  std::istringstream iss(remotepath);
  std::vector<std::string> ancestry;

  while (std::getline(iss, ldir, '/')) {
    ancestry.push_back(ldir);
  }

  return ancestry;
}

} // anonymous namespace

AfsClient::AfsClient(const char *serveraddr, const char *cachedir)
    : stub_{makeStub(serveraddr)}, cachedir_{sanitize(cachedir)} {}

DIR *AfsClient::fuseOpenDir(std::string const &remotepath) {
  ClientContext context;
  Path path;
  path.set_name(remotepath);
  auto statdata = new StatData;
  Status status = stub_->GetFileStat(&context, path, statdata);
  if (!status.ok()) {
    // Emulate server on server failures
    return NULL;
  }

  int nfiles = statdata->dd().files_size();
  // DEBUG("nfiles is %d", nfiles);

  return (DIR *)statdata;
}

int AfsClient::makeParentDirs(std::string const &remotepath) {
  auto ancestry = getAncestry(remotepath);
  int ndirs = ancestry.size() - 1;
  std::string localpath = cachedir_;
  for (int i = 0; i < ndirs; i++) {
    localpath += '/';
    localpath += ancestry[i];

    // ASSUMPTION: symlink exists => hardlink exists
    //   this assmuption is valid when both symbolic and hard links exist in the
    //   cache and symbolic link removed before the hard link
    struct stat statbuf;
    int ret = lstat(localpath.c_str(), &statbuf);
    // - There is no entry
    // - File is not a directory
    if ((ret < 0 && errno == ENOENT) || !S_ISDIR(statbuf.st_mode)) {
      // Remove any existing files to create a directory
      unlink(localpath.c_str());
      // Creating a directory is safe since we have confirmation from server
      // that this is a directory
      if (mkdir(localpath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
        D(perror(__FILE__ ":" EXPAND(__LINE__));)
        return -1;
      }
    }
  }

  return 0;
}

int AfsClient::fetchRegular(std::string const &remotepath, AuthData& authdata) {
  char tmpfname[] = "/tmp/afs/client/fetchreg.XXXXXX";
  int tmpfd = mkstemp(tmpfname);

  Path path;
  path.set_name(remotepath);
  FileData data;
  ClientContext context;

  std::unique_ptr<ClientReader<FileData>> reader(stub_->Fetch(&context, path));

  int fetch_err_code = 0;
  while (reader->Read(&data)) {
    if (!data.status().success()) {
      errno = data.status().err_code();
      D(perror(__FILE__ ":" EXPAND(__LINE__));)
      fetch_err_code = errno;
      break;
    } else {
      if (write(tmpfd, data.contents().c_str(), data.contents().size()) < 0) {
        D(perror(__FILE__ ":" EXPAND(__LINE__));)
        fetch_err_code = errno;
        break;
      }
    }
  }

  // Done writing to temporary file
  close(tmpfd);

  Status status = reader->Finish();
  if (!status.ok()) {
    // Emulate server on server failure
    unlink(tmpfname);
    return 0;
  }

  // Do not persist fetched data on failure
  // Send a finish to the server to indicate completion though
  if (fetch_err_code) {
    errno = fetch_err_code;
    return -1;
  }

  std::string localpath = concatenatedPaths(cachedir_, remotepath);

  if (rename(tmpfname, localpath.c_str()) < 0) {
    D(perror(__FILE__ ":" EXPAND(__LINE__));)
    int err_code = errno;
    unlink(tmpfname);
    errno = err_code;
    return -1;
  }
  int chmod_ret =
      chmod(localpath.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH);
  if (chmod_ret < 0) {
    DEBUG("Failed to change permission of tmp file\n");
  }

  // Update our timestamp based on returned timestamps
  struct timespec times[2];
  // Don't change the last access time...
  times[0].tv_nsec = UTIME_OMIT;
  // Set last modified time to be the same as server
  times[1].tv_nsec = authdata.lmtime().nanos();
  times[1].tv_sec = authdata.lmtime().seconds();
  if (utimensat(-1, localpath.c_str(), times, 0) < 0) {
    D(perror(__FILE__ ":" EXPAND(__LINE__));)
    return -1;
  }

  return 0;
}

int AfsClient::fetchDirectory(std::string const &remotepath) {
  auto localpath = concatenatedPaths(cachedir_, remotepath);
  char tmpfname[] = "/tmp/afs/client/fetchdir.XXXXXX";
  if (!mkdtemp(tmpfname)) {
    D(perror(__FILE__ ":" EXPAND(__LINE__));)
    return -1;
  }
  std::string tmppath = std::string(tmpfname);

  // Empty directory created for the purposes of swapping
  char sinkfname[] = "/tmp/afs/client/sinkdir.XXXXXX";
  if (!mkdtemp(sinkfname)) {
    int err_code = errno;
    rmdir(tmpfname);
    errno = err_code;
    D(perror(__FILE__ ":" EXPAND(__LINE__));)
    return -1;
  }


  // Here we use an empty directory to sink our current directory
  // We avoid failing when the localpath is not yet created
  if (rename(localpath.c_str(), sinkfname) < 0 && errno != ENOENT) {
    D(perror(__FILE__ ":" EXPAND(__LINE__));)
    int err_code = errno;
    // XXX: tmpfname may be non-empty => not removed
    rmdir(tmpfname);
    rmdir(sinkfname);
    errno = err_code;
    return -1;
  }

  if (rename(tmpfname, localpath.c_str()) < 0) {
    D(perror(__FILE__ ":" EXPAND(__LINE__));)
    int err_code = errno;
    // XXX: tmpfname, sinkfname may be non-empty => not removed
    rmdir(tmpfname);
    rmdir(sinkfname);
    errno = err_code;
    return -1;
  }

  // XXX: sinkfname may be non-empty => not removed
  rmdir(sinkfname);

  return 0;
}

int AfsClient::Fetch(std::string const &remotepath) {
  std::string const localpath = concatenatedPaths(cachedir_, remotepath);

  // Ensure we can fetch the file
  auto authdata = TestAuth(remotepath);
  if (!authdata.status().success()) {
    // XXX: Should we remove entry from cache?
    errno = authdata.status().err_code();
    D(perror(__FILE__ ":" EXPAND(__LINE__));)
    return -1;
  }

  struct stat statbuf;
  int ret = lstat(localpath.c_str(), &statbuf);

  // Check if we are on the latest version of the file
#ifdef __APPLE__
  if (ret >= 0 && authdata.lmtime().seconds() <= statbuf.st_mtime) {
#else
  if (ret >= 0 && authdata.lmtime().seconds() < statbuf.st_mtim.tv_sec || 
  ((authdata.lmtime().seconds() == statbuf.st_mtim.tv_sec) && 
  (authdata.lmtime().nanos() <= statbuf.st_mtim.tv_nsec))) {
#endif
    return 0;
  }

  // First, we make all the parent directories
  if (ret < 0 && makeParentDirs(remotepath) < 0) {
    D(perror(__FILE__ ":" EXPAND(__LINE__));)
    return -1;
  }

  // XXX: Server cannot provide the precise value of mode
  //   and hence we do not call chmod
  // Then, we fetch the regular file or directory depending on the mode
  if (S_ISREG(authdata.mode())) {
    return fetchRegular(remotepath, authdata);
  }

  if (S_ISDIR(authdata.mode())) {
    if (ret < 0) {
      return fetchDirectory(remotepath);
    } else {
      return 0;
    }
  }

  // special type of file - do not support
  errno = ENOTSUP;
  return -1;
}

int AfsClient::Store(std::string const &remotepath) {
  ClientContext context;

  // We use the result to update our timestamps
  StoreResult result;
  std::unique_ptr<ClientWriter<StoreData>> writer(
      stub_->Store(&context, &result));

  // First send the pathname of the file
  StoreData metadata;
  auto path = metadata.mutable_path();
  path->set_name(remotepath);
  if (!writer->Write(metadata)) {
    // Emulate the server when server crashes
    return 0;
  }

  // Open file for reading
  auto localpath = concatenatedPaths(cachedir_, remotepath);
  auto fp = fopen(localpath.c_str(), "r");
  if (!fp) {
    D(perror(__FILE__ ":" EXPAND(__LINE__));)
    return -1;
  }

  int store_err_code = 0;
  while (!feof(fp)) {
    int datalen = fread(iobuf, 1, sizeof(iobuf), fp);
    if (ferror(fp)) {
      fclose(fp);
      D(perror(__FILE__ ":" EXPAND(__LINE__));)
      store_err_code = errno;

      StoreData errordata;
      auto status = errordata.mutable_filedata()->mutable_status();
      status->set_success(false);
      status->set_err_code(store_err_code);
      errordata.set_laststore(false);

      writer->Write(errordata);
      break;
    }

    StoreData storedata;

    auto filedata = storedata.mutable_filedata();
    filedata->set_contents(std::string(iobuf, datalen));

    auto status = filedata->mutable_status();
    status->set_success(true);
    storedata.set_laststore(feof(fp));

    if (!writer->Write(storedata)) {
      // Emulate the server when server crashes
      fclose(fp);
      return 0;
    }
  }

  // Close read file
  fclose(fp);

  writer->WritesDone();
  Status status = writer->Finish();
  if (!status.ok()) {
    // Emulate the server when server crashes
    return 0;
  }

  // Do not update timestamp when we encounter errors
  // Still need to send a writesdone and finish to the server though
  if (store_err_code) {
    errno = store_err_code;
    return -1;
  }

  // Update our timestamp based on returned timestamps
  struct timespec times[2];
  // Don't change the last access time...
  times[0].tv_nsec = UTIME_OMIT;
  // Set last modified time to be the same as server
  times[1].tv_nsec = result.lmtime().nanos();
  times[1].tv_sec = result.lmtime().seconds();
  if (utimensat(-1, localpath.c_str(), times, 0) < 0) {
    D(perror(__FILE__ ":" EXPAND(__LINE__));)
    return -1;
  }
  return 0;
}

int AfsClient::Remove(std::string const &remotepath) {
  ClientContext context;
  Path path;
  path.set_name(remotepath);

  IOResult result;
  Status status = stub_->Remove(&context, path, &result);
  if (!status.ok()) {
    // Emulate server on server failure
    return 0;
  }

  if (!result.success()) {
    errno = result.err_code();
    D(perror(__FILE__ ":" EXPAND(__LINE__));)
    return -1;
  }

  return 0;
}

int AfsClient::Create(std::string const &remotepath) {
  ClientContext context;
  Path path;
  path.set_name(remotepath);

  IOResult result;

  Status status = stub_->Create(&context, path, &result);
  if (!status.ok()) {
    // Emulate server on server failure
    return 0;
  }

  if (!result.success()) {
    errno = result.err_code();
    D(perror(__FILE__ ":" EXPAND(__LINE__));)
    return -1;
  }

  return 0;
}

int AfsClient::Rename(std::string const &oldpath, std::string const &newpath) {
  ClientContext context;

  RenameArgs args;
  args.mutable_oldname()->set_name(oldpath);
  args.mutable_newname()->set_name(newpath);

  IOResult result;
  Status status = stub_->Rename(&context, args, &result);
  if (!status.ok()) {
    // Emulate server on server failure
    return 0;
  }

  if (!result.success()) {
    errno = result.err_code();
    D(perror(__FILE__ ":" EXPAND(__LINE__));)
    return -1;
  }

  return 0;
}

int AfsClient::Makedir(std::string const &remotepath) {
  ClientContext context;
  Path path;
  path.set_name(remotepath);

  IOResult result;

  Status status = stub_->Makedir(&context, path, &result);
  if (!status.ok()) {
    // Emulate server on server failures
    return 0;
  }

  if (!result.success()) {
    errno = result.err_code();
    D(perror(__FILE__ ":" EXPAND(__LINE__));)
    return -1;
  }

  return 0;
}

int AfsClient::Removedir(std::string const &remotepath) {
  ClientContext context;
  Path path;
  path.set_name(remotepath);

  IOResult result;
  Status status = stub_->Removedir(&context, path, &result);
  if (!status.ok()) {
    // Emulate server on server failures
    return 0;
  }

  if (!result.success()) {
    errno = result.err_code();
    D(perror(__FILE__ ":" EXPAND(__LINE__));)
    return -1;
  }

  return 0;
}

AuthData AfsClient::TestAuth(std::string const &remotepath) {
  ClientContext context;
  Path path;
  path.set_name(remotepath);

  AuthData authdata;
  Status status = stub_->TestAuth(&context, path, &authdata);
  if (!status.ok()) {
    // Emulate server on server failures
    auto localpath = concatenatedPaths(cachedir_, remotepath);
    struct stat statbuf;
    auto status = authdata.mutable_status();
    if (lstat(localpath.c_str(), &statbuf) < 0) {
      status->set_err_code(errno);
      status->set_success(false);
      return authdata;
    }
    status->set_success(true);
    auto lmtime = authdata.mutable_lmtime();
#ifdef __APPLE__
    lmtime->set_seconds(statbuf.st_mtime);
    lmtime->set_nanos(0);
#else
    lmtime->set_seconds(statbuf.st_mtim.tv_sec);
    lmtime->set_nanos(statbuf.st_mtim.tv_nsec);
#endif
    authdata.set_mode(statbuf.st_mode);
    return authdata;
  }

  return authdata;
}

/////////////////
// FUSE interface
/////////////////

std::string AfsClient::getAFSPath(std::string const &localpath) {
  return localpath.substr(cachedir_.size(), std::string::npos);
}

int AfsClient::fuse_lstat(const char *path, struct stat *buf) {
  if (Fetch(getAFSPath(path)) < 0) {
    return -1;
  }

  return lstat(path, buf);
}

int AfsClient::fuse_mkdir(const char *path, mode_t mode) {
  if (Makedir(getAFSPath(path)) < 0) {
    return -1;
  }

  return 0;
}

int AfsClient::fuse_rmdir(const char *path) {
  if (Removedir(getAFSPath(path)) < 0) {
    return -1;
  }

  return rmdir(path);
}

int AfsClient::fuse_rename(const char *oldpath, const char *newpath) {
  if (Rename(getAFSPath(oldpath), getAFSPath(newpath)) < 0) {
    return -1;
  }

  return rename(oldpath, newpath);
}

int AfsClient::fuse_truncate(const char *path, off_t length) {
  if (truncate(path, length) < 0) {
    return -1;
  }
  auto remotePath = getAFSPath(path);
  return Store(remotePath);
}

int AfsClient::fuse_open(const char *path, int flags) {
  if (flags & O_CREAT) {
    return fuse_creat(path, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  }

  if (Fetch(getAFSPath(path)) < 0) {
    return -1;
  }

  flags &= ~O_EXCL;
  return open(path, flags);
}

int AfsClient::fuse_creat(const char *path, int flags, mode_t mode) {
  auto remotepath = getAFSPath(path);

  if (Create(remotepath) < 0) {
    return -1;
  }

  if (Fetch(remotepath) < 0) {
    return -1;
  }

  flags &= ~O_EXCL;
  return open(path, flags, mode);
}

int AfsClient::fuse_statvfs(const char *path, struct statvfs *buf) {
  if (Fetch(getAFSPath(path)) < 0) {
    return -1;
  }

  return statvfs(path, buf);
}

DIR *AfsClient::fuse_opendir(const char *path) {
  if (Fetch(getAFSPath(path)) < 0) {
    return NULL;
  }

  return fuseOpenDir(getAFSPath(path));
}

int AfsClient::fuse_close(int fd, const char *path, bool receive_write) {
  if (close(fd) < 0) {
    D(perror(__FILE__ ":" EXPAND(__LINE__));)
    return -1;
  }

  if (receive_write) {
    auto remotePath = getAFSPath(path);
    return Store(remotePath);
  }
  return 0;
}

int AfsClient::fuse_unlink(const char *path) {
  if (Remove(getAFSPath(path)) < 0) {
    return -1;
  }

  return unlink(path);
}

int AfsClient::fuse_access(const char *path, int amode) {
  if (Fetch(getAFSPath(path)) < 0) {
    return -1;
  }

  return access(path, amode);
}

// global variable to maintain the connection state to the server
std::unique_ptr<AfsClient> g_afsClient = nullptr;

void afs_client_init() {
    std::string address("0.0.0.0:5000");
    g_afsClient = std::make_unique<AfsClient>(
        grpc::CreateChannel(
            address,
            grpc::InsecureChannelCredentials()
        )
    );
}

void afs_client_destroy() {
    g_afsClient.reset();
}

// Flags
// - O_CREAT - creates the file if not present
// - O_RDONLY
// - O_WRONLY
// - O_RDWR
//
// Returns
// - a file descriptor if successful
// - an error otherwise
int afs_client_open(const char* filepath, int flags) {
    if (flags != O_RDONLY) return -ENOTSUP;

    int ret;

    // check for cached file
    ret = open(filepath, O_RDONLY);
    if (ret >= 0) return ret;

    // file not cached
    // - open a file on local fs for read-write
    // - read file from remote
    // - write file to local
    // - reset file offset
    ret = open(filepath, O_RDWR);
    if (ret < 0) return -errno;
    int fd = ret;

    // remote fetch
    auto response = g_afsClient->Fetch(std::string(filepath));

    // local write
    ret = write(fd, response.c_str(), response.size());
    if (ret < 0) return -errno;

    // reset
    ret = lseek(fd, 0, SEEK_SET);
    if (ret < 0) return -errno;

    return fd;
}
