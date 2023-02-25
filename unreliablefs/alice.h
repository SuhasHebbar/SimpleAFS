#pragma once

#include <sys/types.h>

void alice_init();
void alice_destroy();
int alice_delay_pwrite(int fd, const char *buf, size_t size, off_t offset);
int alice_reorder_pwrite(int fd, const char *buf, size_t size, off_t offset);
void alice_fsync(int fd);
