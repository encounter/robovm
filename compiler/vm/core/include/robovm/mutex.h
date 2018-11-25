/*
 * Copyright (C) 2012 RoboVM AB
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef ROBOVM_MUTEX_H
#define ROBOVM_MUTEX_H

#ifdef HORIZON
#include "switch_pthread.h"
#else
#include <pthread.h>
#endif

/* Mutex wrappers around pthread mutexes */
static inline jint rvmInitMutex(RvmMutex* mutex) {
    pthread_mutexattr_t mutexAttrs;
    pthread_mutexattr_init(&mutexAttrs);
    pthread_mutexattr_settype(&mutexAttrs, PTHREAD_MUTEX_RECURSIVE);
    return pthread_mutex_init(mutex, &mutexAttrs);
}
static inline jint rvmDestroyMutex(RvmMutex* mutex) {
    return pthread_mutex_destroy(mutex);
}
static inline jint rvmLockMutex(RvmMutex* mutex) {
    return pthread_mutex_lock(mutex);
}
static inline jint rvmTryLockMutex(RvmMutex* mutex) {
    return pthread_mutex_trylock(mutex);
}
static inline jint rvmUnlockMutex(RvmMutex* mutex) {
    return pthread_mutex_unlock(mutex);
}

#endif
