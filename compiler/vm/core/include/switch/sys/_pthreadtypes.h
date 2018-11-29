#pragma once

#include <sys/sched.h>

typedef struct pthread *pthread_t;            /* identify a thread */

/* P1003.1c/D10, p. 118-119 */
#define PTHREAD_SCOPE_PROCESS 0
#define PTHREAD_SCOPE_SYSTEM  1

/* P1003.1c/D10, p. 111 */
#define PTHREAD_INHERIT_SCHED  1      /* scheduling policy and associated */
/*   attributes are inherited from */
/*   the calling thread. */
#define PTHREAD_EXPLICIT_SCHED 2      /* set from provided attribute object */

/* P1003.1c/D10, p. 141 */
#define PTHREAD_CREATE_DETACHED 0
#define PTHREAD_CREATE_JOINABLE  1

typedef struct {
    int is_initialized;
    void *stackaddr;
    size_t stacksize;
    int contentionscope;
    int inheritsched;
    int schedpolicy;
    struct sched_param schedparam;
    int  detachstate;
} pthread_attr_t;

typedef struct pthread_mutex *pthread_mutex_t;      /* identify a mutex */

typedef struct {
    int   is_initialized;
    int   recursive;
} pthread_mutexattr_t;

#define _PTHREAD_MUTEX_INITIALIZER ((pthread_mutex_t) 0xFFFFFFFF)

/* Condition Variables */

typedef __uint32_t pthread_cond_t;       /* identify a condition variable */

#define _PTHREAD_COND_INITIALIZER ((pthread_cond_t) 0xFFFFFFFF)

typedef struct {
    int      is_initialized;
    clock_t  clock;             /* specifiy clock for timeouts */
} pthread_condattr_t;         /* a condition attribute object */

/* Keys */

typedef __uint32_t pthread_key_t;        /* thread-specific data keys */

typedef struct {
    int   is_initialized;  /* is this structure initialized? */
    int   init_executed;   /* has the initialization routine been run? */
} pthread_once_t;       /* dynamic package initialization */

#define _PTHREAD_ONCE_INIT  { 1, 0 }  /* is initialized and not run */
