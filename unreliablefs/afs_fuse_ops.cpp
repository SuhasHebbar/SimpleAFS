#include <cerrno>
#include <memory>
#include <thread>
#include <pthread.h>

#include <fuse.h>
#include <unistd.h>

#include "afs_client.h"

namespace {

std::unique_ptr<AfsClient> g_afsClient;

enum class FileState {
  INVALID_HANDLE = 0,
  VALID_HANDLE,
  RECEIVED_WRITE,
};

std::mutex fuse_lock;
// Trace if an open file handle received a write.
std::array<FileState, 1024> file_write_state;

// TODO: Create utils.cpp to store utility functions like this.
int ensureEmptyDirectory(std::string const& path) {
  char tmpfname[] = "/tmp/afs/client/fetchdir.XXXXXX";
  if (!mkdtemp(tmpfname)) {
    return -1;
  }
  std::string tmppath = std::string(tmpfname);

  // Empty directory created for the purposes of swapping
  char sinkfname[] = "/tmp/afs/client/sinkdir.XXXXXX";
  if (!mkdtemp(sinkfname)) {
    int err_code = errno;
    rmdir(tmpfname);
    errno = err_code;
    return -1;
  }


  // Here we use an empty directory to sink our current directory
  // We avoid failing when the localpath is not yet created
  if (rename(path.c_str(), sinkfname) < 0 && errno != ENOENT) {
    int err_code = errno;
    // XXX: tmpfname may be non-empty => not removed
    rmdir(tmpfname);
    rmdir(sinkfname);
    errno = err_code;
    return -1;
  }

  if (rename(tmpfname, path.c_str()) < 0) {
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

void backgroundHeartbeat(std::string const& cachedir) {
  while (true) {
    if (!g_afsClient->server_alive()) {
      ensureEmptyDirectory(cachedir);
    }
    sleep(5);
  }

}

} // anonymous namespace

extern "C" {

int afs_fuse_setup(const char *serveraddr, const char *cachedir) {
  // Lock is not necessary but let's keep it a good habit.
  fuse_lock.lock();
  // Initialize even if globally it would already initialized.
  std::fill(file_write_state.begin(), file_write_state.end(),
            FileState::INVALID_HANDLE);
  fuse_lock.unlock();

  if (mkdir("/tmp/afs", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0 &&
      errno != EEXIST) {
    return -1;
  }

  if (mkdir("/tmp/afs/client", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0 &&
      errno != EEXIST) {
    return -1;
  }

  ensureEmptyDirectory(cachedir);

  g_afsClient = std::make_unique<AfsClient>(serveraddr, cachedir);

  std::thread background_thread(backgroundHeartbeat, cachedir);
  pthread_setname_np(background_thread.native_handle(), "background_hearbeat");
  
  return 0;
}

void afs_fuse_teardown() { g_afsClient.reset(); }

int afs_fuse_lstat(const char *path, struct stat *buf) {

  return g_afsClient->fuse_lstat(path, buf);
}

int afs_fuse_mkdir(const char *path, mode_t mode) {

  return g_afsClient->fuse_mkdir(path, mode);
}

int afs_fuse_rmdir(const char *path) { return g_afsClient->fuse_rmdir(path); }

int afs_fuse_rename(const char *oldpath, const char *newpath) {

  return g_afsClient->fuse_rename(oldpath, newpath);
}

int afs_fuse_truncate(const char *path, off_t length) {
  int ret = g_afsClient->fuse_truncate(path, length);
  return ret;
}

int afs_fuse_open(const char *path, int flags) {

  int fd = g_afsClient->fuse_open(path, flags);
  if (fd != -1) {
    fuse_lock.lock();
    file_write_state[fd] = FileState::VALID_HANDLE;
    fuse_lock.unlock();
  }

  return fd;
}

int afs_fuse_creat(const char *path, int flags, mode_t mode) {
  int fd =  g_afsClient->fuse_creat(path, flags, mode);

  if (fd != -1) {
    fuse_lock.lock();
    file_write_state[fd] = FileState::VALID_HANDLE;
    fuse_lock.unlock();
  }

  return fd;
}

int afs_fuse_statvfs(const char *path, struct statvfs *buf) {

  return g_afsClient->fuse_statvfs(path, buf);
}

DIR *afs_fuse_opendir(const char *path) {

  return g_afsClient->fuse_opendir(path);
}

int afs_fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                     off_t offset, struct fuse_file_info *fi) {

  DIR *dp = g_afsClient->fuse_opendir(path);
  if (dp == nullptr) {
    return -errno;
  }

  (void)offset;
  (void)fi;

  std::unique_ptr<afs::StatData> statdata{(afs::StatData *)dp};

  int nfiles = statdata->dd().files_size();
  for (int i = 0; i < nfiles; i++) {
    auto entry = statdata->dd().files(i);
    // if (entry == "." || entry == "..") {
    //   continue;
    // }
    struct stat st;
    memset(&st, 0, sizeof(st));
    st.st_ino = 0;
    // st.st_mode = de->d_type << 12;
    if (filler(buf, entry.c_str(), &st, 0))
      break;
  }

  return 0;
}

int afs_fuse_closedir(DIR *dp) {
  if (dp == NULL) {
    return -1;
  }

  std::unique_ptr<afs::StatData> statdata{(afs::StatData *)dp};
  return 0;
}

int afs_fuse_close(int fd, const char *path) {
  fuse_lock.lock();
  bool file_received_write = file_write_state[fd] == FileState::RECEIVED_WRITE;
  file_write_state[fd] = FileState::INVALID_HANDLE;
  fuse_lock.unlock();

  return g_afsClient->fuse_close(fd, path, file_received_write);
}

void afs_fuse_markdirty(int fd) {
  if (fd >= 0 && fd < 1024) {
    fuse_lock.lock();
    file_write_state[fd] = FileState::RECEIVED_WRITE;
    fuse_lock.unlock();
  }
}

void afs_fuse_copydirty(int srcfd, int destfd) {
  fuse_lock.lock();
  file_write_state[destfd] = file_write_state[srcfd];
  fuse_lock.unlock();
}

int afs_fuse_unlink(const char *path) { return g_afsClient->fuse_unlink(path); }

int afs_fuse_access(const char *path, int amode) {

  return g_afsClient->fuse_access(path, amode);
}

} // extern "C"
