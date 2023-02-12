#pragma once

void afs_init();
void afs_destroy();
int afs_open(const char* pathname, int flags);
