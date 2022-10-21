#ifndef LOCK_H
#define LOCK_H

#include <include/task.h>
#include <include/types.h>

typedef struct _mutex_t {
    int32_t volatile lock;
    pid_t volatile owner;
    bool volatile init;
} mutex_t;

enum _mutex_lock_state { MUTEX_LOCKED, MUTEX_UNLOCKED };
enum _mutex_error_type {
    /*
     * https://linux.die.net/man/3/pthread_mutex_lock
     *
     * The mutex_lock(), mutex_trylock(), and mutex_unlock(), mutex_init(),
     * mutex_destroy() functions may fail if:
     *
     * mutex_lock(),
     * mutex_trylock(),
     * mutex_unlock() - does not refer to an initialized mutex object.
     * mutex_destroy(), mutex_init() - The value specified by mutex is invalid.
     */
    EINVAL = 1,
    /*
     * The mutex_init(), mutex_destroy(), mutex_trylock() function may fail if:
     *
     * mutex_init() - The implementation has detected an attempt to reinitialize
     *                the object referenced by mutex
     * mutex_destroy() - The implementation has detected an attempt to destroy
     *                   the object referenced by mutex while it is locked (does
     *                   not considering referenced or not)
     * mutex_trylock() - The mutex could not be acquired because it was already
     *                   locked.
     */
    EBUSY,
    /*
     * The mutex_lock() function may fail if:
     */
    EDEADLK,  // The current thread already owns the mutex.
    /*
     The mutex_unlock() function may fail if:
      */
    EPERM,  // The current thread does not own the mutex.
};

int mutex_init(mutex_t *mutex);
int mutex_destroy(mutex_t *mutex);
int mutex_lock(mutex_t *mutex);
int mutex_trylock(mutex_t *mutex);
int mutex_unlock(mutex_t *mutex);

#endif