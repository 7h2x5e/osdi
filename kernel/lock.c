#include <include/irq.h>
#include <include/lock.h>
#include <include/sched.h>

/*
 * If successful, the mutex_init() and mutex_destroy() functions shall return
 * zero, otherwise, an error number shall be returned to indicate the error.
 */
int mutex_init(mutex_t *mutex)
{
    int ret = 0;
    disable_irq();
    { /* critical section */
        if (!mutex) {
            ret = EINVAL;
        } else if (mutex->init) {
            ret = EBUSY;  // reinitialize
        } else {
            mutex->init = true;
            mutex->lock = MUTEX_UNLOCKED;
            mutex->owner = 0;
        }
    }
    enable_irq();
    return ret;
}

int mutex_destroy(mutex_t *mutex)
{
    int ret = 0;
    disable_irq();
    {
        /* critical section */
        if (!mutex || !mutex->init) {
            ret = EINVAL;
        } else if (mutex->lock != MUTEX_UNLOCKED) {
            ret = EBUSY;
        } else {
            mutex->init = false;
        }
    }
    enable_irq();
    return ret;
}

/*
 * If successful, the mutex_lock() ,mutex_unlock(), mutex_trylock() functions
 * shall return zero, otherwise, an error number shall be returned to indicate
 * the error.
 */
int mutex_lock(mutex_t *mutex)
{
    pid_t pid = do_get_taskid();
    disable_irq();
    {
        /* critical section */
        if (!mutex || !mutex->init) {
            enable_irq();
            return EINVAL;
        }
        while (mutex->lock == MUTEX_LOCKED) {
            if (mutex->owner == pid) {
                enable_irq();
                return EDEADLK;
            }
            enable_irq();
            schedule();  // put current task into runqueue to wait for a lock to
                         // be released
            disable_irq();
        }
        mutex->owner = pid;
        mutex->lock = MUTEX_LOCKED;
    }
    enable_irq();
    return 0;
}

/*
 * shall return zero if a lock on the mutex object referenced by mutex is
 * acquired. Otherwise, an error number is returned to indicate the error.
 */
int mutex_trylock(mutex_t *mutex)
{
    pid_t pid = do_get_taskid();
    disable_irq();
    {
        /* critical section */
        if (!mutex || !mutex->init) {
            enable_irq();
            return EINVAL;
        }

        if (mutex->lock == MUTEX_LOCKED) {
            enable_irq();
            return EBUSY;
        }
        mutex->owner = pid;
        mutex->lock = MUTEX_LOCKED;
    }
    enable_irq();
    return 0;
}

int mutex_unlock(mutex_t *mutex)
{
    pid_t pid = do_get_taskid();
    disable_irq();
    {
        /* critical section */
        if (!mutex || !mutex->init) {
            enable_irq();
            return EINVAL;
        }
        if (mutex->owner != pid) {
            enable_irq();
            return EPERM;
        }
        mutex->owner = 0;
        mutex->lock = MUTEX_UNLOCKED;
    }
    enable_irq();
    return 0;
}
