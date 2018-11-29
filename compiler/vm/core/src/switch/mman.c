#include <sys/mman.h>
#include <stdio.h>
#include <malloc.h>

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    printf("[WARN] Called unimplemented function mmap()");
    return malloc(length);
}

int munmap(void *addr, size_t length) {
    printf("[WARN] Called unimplemented function munmap()");
    free(addr);
    return 0;
}

int madvise(void *addr, size_t length, int advice) {
    printf("[WARN] Called unimplemented function madvise()");
    return 0;
}

int mlock(const void *addr, size_t len) {
    printf("[WARN] Called unimplemented function mlock()");
    return 0;
}

int munlock(const void *addr, size_t len) {
    printf("[WARN] Called unimplemented function munlock()");
    return 0;
}

int msync(void *addr, size_t length, int flags) {
    printf("[WARN] Called unimplemented function msync()");
    return 0;
}

int mprotect(void *addr, size_t len, int prot) {
    printf("[WARN] Called unimplemented function mprotect()");
    return 0;
}