#include <errno.h>
#include <malloc.h>
#include <string.h>

#include <switch/result.h>
#include <switch/types.h>
#include <switch/arm/tls.h>
#include <switch/kernel/condvar.h>
#include <switch/kernel/mutex.h>
#include <switch/kernel/svc.h>
#include <switch/kernel/thread.h>
#include <switch/kernel/virtmem.h>

#include "horizon/pthread.h"

#define ATTR_MAGIC 0x21545625
#define PTHREAD_KEYS_MAX 32
#define PTHREAD_MUTEX_RECURSIVE 1

typedef struct {
    /* Sequence numbers.  Even numbers indicate vacant entries.  Note
       that zero is even.  We use uintptr_t to not require padding.  */
    uintptr_t seq;

    void (*destr)(void *);
} pthread_key_struct;

static pthread_key_struct __pthread_keys[PTHREAD_KEYS_MAX] = {};

typedef CondVar pthread_cond_t;

struct pthread {
    Thread thr;
    int rc;

    struct pthread_key_data {
        uintptr_t seq;
        void *data;
    } specific[PTHREAD_KEYS_MAX];
};

struct pthread_mutex {
    int type;
    union {
        Mutex mutex;
        RMutex rmutex;
    };
};

struct pthread_attr {
    int is_initialized;
    void *stackaddr;
    size_t stacksize;
    struct sched_param schedparam;
    int detachstate;
};

struct pthread_mutexattr {
    int is_initialized;
    int type;
};

struct pthread_condattr {
    int is_initialized;
};

#define THREADVARS_MAGIC 0x21545624 // !TV$

// FIXME need to keep in sync with nx/source/internal.h
typedef struct {
    // Magic value used to check if the struct is initialized
    u32 magic;

    // Thread handle, for mutexes
    Handle handle;

    // Pointer to the current thread (if exists)
    Thread *thread_ptr;

    // Pointer to this thread's newlib state
    struct _reent *reent;

    // Pointer to this thread's thread-local segment
    void *tls_tp; // !! Offset needs to be TLS+0x1F8 for __aarch64_read_tp !!
} ThreadVars;

static inline ThreadVars *getThreadVars(void) {
    return (ThreadVars *) ((u8 *) armGetTls() + 0x1E0);
}

pthread_t pthread_self(void) {
    ThreadVars *vars = getThreadVars();
    Thread *t = vars->magic == THREADVARS_MAGIC ? vars->thread_ptr : NULL;
    // We assume pthread_t is a struct whose first entry is Thread
    // So we can cast it back to get our info
    return t ? (pthread_t) t : THRD_MAIN_HANDLE;
}

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
                       pthread_mutex_t *__restrict mutex,
                       uint64_t timeout) {
    if (!cond || !mutex) return EINVAL;
    struct pthread_mutex *mtx = *(struct pthread_mutex **) mutex;

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

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
    if (!mutex || !attr || !attr->is_initialized) return EINVAL;
    struct pthread_mutex *mtx = *(struct pthread_mutex **) mutex;

    mtx->type = 0;
    if (attr->recursive) mtx->type |= PTHREAD_MUTEX_RECURSIVE;
    if (mtx->type & PTHREAD_MUTEX_RECURSIVE) {
        rmutexInit(&mtx->rmutex);
    } else {
        mutexInit(&mtx->mutex);
    }
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
    if (!mutex) return EINVAL;
    struct pthread_mutex *mtx = *(struct pthread_mutex **) mutex;

    if (mtx->type & PTHREAD_MUTEX_RECURSIVE) {
        rmutexLock(&mtx->rmutex);
    } else {
        mutexLock(&mtx->mutex);
    }
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    if (!mutex) return EINVAL;
    struct pthread_mutex *mtx = *(struct pthread_mutex **) mutex;

    if (mtx->type & PTHREAD_MUTEX_RECURSIVE) {
        rmutexUnlock(&mtx->rmutex);
    } else {
        mutexUnlock(&mtx->mutex);
    }
    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    if (!mutex) return EINVAL;
    struct pthread_mutex *mtx = *(struct pthread_mutex **) mutex;

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

_Noreturn void pthread_exit(__unused void *retval) {
    pthread_t t = pthread_self();
    t->rc = *(int *) retval; // FIXME not right...
    svcExitThread();
}

/////// SPECIFICS

int pthread_key_create(pthread_key_t *key, destructor_t destructor) {
    // FIXME implement
    return 0;
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

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate) {
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
    attr->recursive = 0;
    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type) {
    if (!attr || attr->is_initialized != ATTR_MAGIC) return EINVAL;
    attr->recursive = (type & PTHREAD_MUTEX_RECURSIVE) == PTHREAD_MUTEX_RECURSIVE;
    return 0;
}

int pthread_mutexattr_setrecursive(pthread_mutexattr_t *attr, int recursive) {
    if (!attr || attr->is_initialized != ATTR_MAGIC) return EINVAL;
    attr->recursive = recursive;
    return 0;
}

/////// COND ATTRS

int pthread_condattr_init(pthread_condattr_t *attr) {
    if (!attr) return EINVAL;
    attr->is_initialized = ATTR_MAGIC;
    return 0;
}

///// SCHED

int sched_yield() {
    svcSleepThread(-1); // FIXME?
    return 0;
}