#pragma once

#define SIGINT 0
#define SIGRTMIN 0

typedef int sig_atomic_t;

struct sigaction {
    void (*sa_handler)(int);
    int sa_flags;
};

int sigaction(int sig, struct sigaction *sa, void *);