#pragma once

// FIXME requires -D_SYS__PTHREADTYPES_H_ to avoid sys/_pthreadtypes.h

#include <time.h>

#define THRD_MAIN_HANDLE ((pthread_t) ~(uintptr_t) 0)

typedef void *(*thrd_start_t)(void *);
typedef void (*destructor_t)(void*);

//enum pthread_mutexattr_type {
//    PTHREAD_MUTEX_DEFAULT = 0,
//    PTHREAD_MUTEX_RECURSIVE = 1
//};

#define SCHED_PRIORITY_MIN 0
#define SCHED_PRIORITY_MAX 0x3F

#define PTHREAD_MUTEX_INITIALIZER NULL

#define PTHREAD_MUTEX_RECURSIVE 1

pthread_t pthread_self();

// FIXME sys/signal.h
//int pthread_kill(pthread_t thread, int signal);

_Noreturn void pthread_exit(void *retval);

/**
 * Mutex
 */

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);

static inline int pthread_mutex_destroy(__unused pthread_mutex_t *mutex) {
    // Nothing
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex);

int pthread_mutex_unlock(pthread_mutex_t *mutex);

int pthread_mutex_trylock(pthread_mutex_t *mutex);

/**
 * Thread
 */

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, thrd_start_t func, void *arg);

inline int pthread_detach(__unused pthread_t thread) {
    // Nothing
    return 0;
}

int pthread_join(pthread_t thread, void **retval);

int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param *);

/**
 * Specifics
 */

int pthread_key_create(pthread_key_t *key, destructor_t destructor);

void *pthread_getspecific(pthread_key_t key);

int pthread_setspecific(pthread_key_t key, void *data);

/**
 * Cond
 */

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);

int pthread_cond_timedwait(pthread_cond_t *__restrict cond,
                           pthread_mutex_t *__restrict mutex,
                           const struct timespec *abstime);

int pthread_cond_init(pthread_cond_t *cond, __unused const pthread_condattr_t *attr);

int pthread_cond_signal(pthread_cond_t *cond);

int pthread_cond_broadcast(pthread_cond_t *cond);

inline int pthread_cond_destroy(pthread_cond_t *cond) {
    // Nothing
    return 0;
}

inline int pthread_equal(pthread_t t1, pthread_t t2) {
    return t1 == t2;
}

/**
 * Thread attrs
 */

int pthread_getattr_np(pthread_t thread, pthread_attr_t *attr);

int pthread_attr_init(pthread_attr_t *attr);

int pthread_attr_getschedpolicy(const pthread_attr_t *attr, int *schedpolicy);

int pthread_attr_getschedparam(const pthread_attr_t *attr, struct sched_param *schedparam);

int pthread_attr_getstack(const pthread_attr_t *attr, void **stackaddr, size_t *stacksize);

int pthread_attr_getguardsize(const pthread_attr_t *attr, size_t *guardsize);

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);

int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize);

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t guardsize);

/**
 * Mutex attrs
 */

int pthread_mutexattr_init(pthread_mutexattr_t *attr);

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);

int pthread_mutexattr_setrecursive(pthread_mutexattr_t *attr, int recursive);

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);

/**
 * Cond attrs
 */

int pthread_condattr_init(pthread_condattr_t *attr);

/**
 * Sched
 */

int sched_yield();

static inline int sched_get_priority_min(__unused int policy) {
    return SCHED_PRIORITY_MIN;
}

static inline int sched_get_priority_max(__unused int policy) {
    return SCHED_PRIORITY_MAX;
}