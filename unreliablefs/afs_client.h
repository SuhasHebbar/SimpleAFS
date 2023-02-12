#pragma once

void afs_client_init();
void afs_client_destroy();
int afs_client_open(const char* open, int flags);
