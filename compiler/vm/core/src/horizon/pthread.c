#include <errno.h>
#include <malloc.h>
#include <string.h>

#include <switch/kernel/svc.h>
#include <switch/kernel/virtmem.h>

#include "horizon/pthread.h"

static pthread_key_struct __pthread_keys[PTHREAD_KEYS_MAX] = {};

typedef struct {
    thrd_start_t func;
    void *arg;

    bool thread_started;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} __pthread_start_info;

int pthread_cond_init(pthread_cond_t *cond, __unused const pthread_condattr_t *attr) {
    if (!cond) return EINVAL;

    condvarInit(cond);
    return 0;
}

static int __cond_wait(pthread_cond_t *__restrict cond,
                       pthread_mutex_t *__restrict mtx,
                       uint64_t timeout) {
    if (!cond || !mtx) return EINVAL;

    uint32_t thread_tag_backup = 0;
    if (mtx->type & PTHREAD_MUTEX_RECURSIVE) {
        if (mtx->rmutex.counter != 1) return EINVAL;
        thread_tag_backup = mtx->rmutex.thread_tag;
        mtx->rmutex.thread_tag = 0;
        mtx->rmutex.counter = 0;
    }

    Result rc = condvarWaitTimeout(cond, &mtx->mutex, timeout);
    if (mtx->type & PTHREAD_MUTEX_RECURSIVE) {
        mtx->rmutex.thread_tag = thread_tag_backup;
        mtx->rmutex.counter = 1;
    }
    return R_SUCCEEDED(rc) ? 0 : rc == 0xEA01 ? ETIMEDOUT : EINVAL;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mtx) {
    return __cond_wait(cond, mtx, UINT64_MAX);
}

int pthread_cond_timedwait(pthread_cond_t *__restrict cond,
                           pthread_mutex_t *__restrict mtx,
                           const struct timespec *abstime) {
    if (!abstime) return EINVAL;
    return __cond_wait(cond, mtx, (u64) abstime->tv_nsec);
}

int pthread_cond_broadcast(pthread_cond_t *cond) {
    if (!cond) return EINVAL;

    Result rc = condvarWakeAll(cond);
    return R_SUCCEEDED(rc) ? 0 : EINVAL;
}

int pthread_cond_signal(pthread_cond_t *cond) {
    if (!cond) return EINVAL;

    Result rc = condvarWakeOne(cond);
    return R_SUCCEEDED(rc) ? 0 : EINVAL;
}

int pthread_mutex_init(pthread_mutex_t *mtx, const pthread_mutexattr_t *attr) {
    if (!mtx || !attr || !attr->is_initialized) return EINVAL;

    mtx->type = attr->type;
    if (mtx->type & PTHREAD_MUTEX_RECURSIVE) {
        rmutexInit(&mtx->rmutex);
    } else {
        mutexInit(&mtx->mutex);
    }
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mtx) {
    if (!mtx) return EINVAL;

    if (mtx->type & PTHREAD_MUTEX_RECURSIVE) {
        rmutexLock(&mtx->rmutex);
    } else {
        mutexLock(&mtx->mutex);
    }
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mtx) {
    if (!mtx) return EINVAL;

    if (mtx->type & PTHREAD_MUTEX_RECURSIVE) {
        rmutexUnlock(&mtx->rmutex);
    } else {
        mutexUnlock(&mtx->mutex);
    }
    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mtx) {
    if (!mtx) return EINVAL;

    bool res;
    if (mtx->type & PTHREAD_MUTEX_RECURSIVE) {
        res = rmutexTryLock(&mtx->rmutex);
    } else {
        res = mutexTryLock(&mtx->mutex);
    }
    return res ? 0 : EINVAL;
}

static void __thrd_entry(void *__arg) {
    __pthread_start_info *info = (__pthread_start_info *) __arg;
    thrd_start_t func = info->func;
    void *arg = info->arg;

    pthread_mutex_lock(&info->mutex);
    info->thread_started = true;
    pthread_cond_signal(&info->cond);
    pthread_mutex_unlock(&info->mutex);

    void *rc = func(arg);
    pthread_exit(rc);
}

int pthread_create(pthread_t *thr, const pthread_attr_t *attr, thrd_start_t func, void *arg) {
    if (!thr || !func) return EINVAL;

    Result rc;
    *thr = NULL;

    u64 core_mask = 0;
    rc = svcGetInfo(&core_mask, 0, CUR_PROCESS_HANDLE, 0);
    if (R_FAILED(rc)) return EPERM;

    pthread_t pt = (pthread_t) malloc(sizeof(struct pthread));
    memset(pt, 0, sizeof(struct pthread));
    if (!pt) return EAGAIN;

    __pthread_start_info info;
    info.func = func;
    info.arg = arg;
    info.thread_started = false;

    pthread_mutexattr_t mtx_attr = {};
    pthread_mutexattr_init(&mtx_attr);

    pthread_condattr_t cond_attr = {};
    pthread_condattr_init(&cond_attr);

    pthread_mutex_init(&info.mutex, &mtx_attr);
    pthread_cond_init(&info.cond, &cond_attr);

    rc = threadCreate(&pt->thr, __thrd_entry, &info, attr->stacksize, attr->schedparam.sched_priority, -2);
    if (R_FAILED(rc))
        goto _error1;

    rc = svcSetThreadCoreMask(pt->thr.handle, -1, (u32) core_mask);
    if (R_FAILED(rc))
        goto _error2;

    rc = threadStart(&pt->thr);
    if (R_FAILED(rc))
        goto _error2;

    pthread_mutex_lock(&info.mutex);
    while (!info.thread_started)
        pthread_cond_wait(&info.cond, &info.mutex);
    pthread_mutex_unlock(&info.mutex);

    *thr = pt;
    return 0;

    _error2:
    threadClose(&pt->thr);
    _error1:
    free(pt);
    return __ELASTERROR; // FIXME
}

int pthread_join(pthread_t thr, void **res) {
    if (!thr || thr == THRD_MAIN_HANDLE) return EINVAL;

    Result rc = threadWaitForExit(&thr->thr);
    if (R_FAILED(rc)) return EINVAL;
    if (res) *(int *) res = thr->rc;

    rc = threadClose(&thr->thr);
    free(thr);

    return R_SUCCEEDED(rc) ? 0 : EINVAL;
}

void pthread_exit(__unused void *retval) {
    pthread_t t = pthread_self();
    t->rc = *(int *) retval; // FIXME not right...
    svcExitThread();
}

/////// SPECIFICS

int pthread_key_create(pthread_key_t *key, destructor_t destructor) {
    // FIXME implement
}

void *pthread_getspecific(pthread_key_t key) {
    if (key > PTHREAD_KEYS_MAX) return NULL;
    pthread_t thread = pthread_self();
    return thread->specific[key].data;
}

int pthread_setspecific(pthread_key_t key, void *data) {
    if (key > PTHREAD_KEYS_MAX) return EINVAL;
    pthread_t thread = pthread_self();
    thread->specific[key].data = data;
    return 0;
}

/////// ATTRS

int pthread_getattr_np(pthread_t thread, pthread_attr_t *attr) {
    if (!thread || !attr) return EINVAL;
    attr->is_initialized = ATTR_MAGIC;
    attr->stackaddr = thread->thr.stack_mem;
    attr->stacksize = thread->thr.stack_sz;
    attr->detachstate = PTHREAD_CREATE_JOINABLE; // FIXME fill in?
    Result rc = svcGetThreadPriority(&attr->schedparam.sched_priority, thread->thr.handle);
    return R_SUCCEEDED(rc) ? 0 : EINVAL;
}

int pthread_attr_init(pthread_attr_t *attr) {
    if (!attr) return EINVAL;
    attr->is_initialized = ATTR_MAGIC;
    attr->stackaddr = NULL;
    attr->stacksize = 128 * 1024;
    attr->schedparam.sched_priority = 0x3B;
    attr->detachstate = PTHREAD_CREATE_JOINABLE;
    return 0;
}

int pthread_attr_getschedpolicy(__unused const pthread_attr_t *attr, __unused int *schedpolicy) {
    // No-op
    return 0;
}

int pthread_attr_getschedparam(const pthread_attr_t *attr, struct sched_param *param) {
    if (!attr || attr->is_initialized != ATTR_MAGIC) return EINVAL;
    *param = attr->schedparam;
    return 0;
}

int pthread_attr_getstack(const pthread_attr_t *attr, void **stackaddr, size_t *stacksize) {
    if (!attr || attr->is_initialized != ATTR_MAGIC) return EINVAL;
    *stackaddr = attr->stackaddr;
    *stacksize = attr->stacksize;
    return 0;
}

int pthread_attr_getguardsize(__unused const pthread_attr_t *attr, __unused size_t *guardsize) {
    // No-op
    return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, enum pthread_attr_detachstate detachstate) {
    if (!attr || attr->is_initialized != ATTR_MAGIC) return EINVAL;
    attr->detachstate = detachstate;
    return 0;
}

int pthread_attr_setguardsize(__unused pthread_attr_t *attr, __unused size_t guardsize) {
    // No-op
    return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize) {
    if (!attr || attr->is_initialized != ATTR_MAGIC) return EINVAL;
    attr->stacksize = stacksize;
    return 0;
}

/////// MUTEX ATTRS

int pthread_mutexattr_init(pthread_mutexattr_t *attr) {
    if (!attr) return EINVAL;
    attr->is_initialized = ATTR_MAGIC;
    attr->type = PTHREAD_MUTEX_DEFAULT;
    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type) {
    if (!attr || attr->is_initialized != ATTR_MAGIC) return EINVAL;
    attr->type = type;
    return 0;
}

/////// COND ATTRS

int pthread_condattr_init(pthread_condattr_t *attr) {
    if (!attr) return EINVAL;
    attr->is_initialized = ATTR_MAGIC;
    return 0;
}
