#pragma once

#include <sys/types.h>

#define PROT_EXEC 0
#define PROT_NONE 0
#define PROT_READ 0
#define PROT_WRITE 0
#define MAP_FIXED 0
#define MAP_PRIVATE 0
#define MAP_SHARED 0
#define MAP_FAILED 0
#define MADV_RANDOM 0
#define OPT_MAP_ANON 0

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);
int madvise(void *addr, size_t length, int advice);
int mlock(const void *addr, size_t len);
int munlock(const void *addr, size_t len);
int msync(void *addr, size_t length, int flags);
int mprotect(void *addr, size_t len, int prot);