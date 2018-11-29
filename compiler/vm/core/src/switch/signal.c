#include <signal.h>
#include <stdio.h>

int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact) {
    printf("[WARN] Called unimplemented function sigaction()!");
    return 0;
}