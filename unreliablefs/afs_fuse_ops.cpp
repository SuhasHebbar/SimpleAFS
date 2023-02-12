#include <cerrno>
#include "afs_client.h"

namespace {
    // Seems redundant but we want to set errno
    int wrap_ret(int ret) {
        if (ret >= 0) return ret;
        errno = ret;
        return -1;
    }
} // anonymous namespace

extern "C" {

// Initialize the client connection
void afs_init() {
    afs_client_init();
}

void afs_destroy() {
    afs_client_destroy();
}

int afs_open(const char* pathname, int flags) {
    return wrap_ret(afs_client_open(pathname, flags));
}

}
