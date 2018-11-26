#pragma once

#define PROT_READ 0
#define PROT_WRITE 0
#define MAP_SHARED 0
#define MAP_FAILED 0

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);