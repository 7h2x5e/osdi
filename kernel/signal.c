#include <include/signal.h>
#include <include/task.h>

/*
 * on success, return 0
 * on error, -1 is returned
 */
int32_t do_kill(pid_t pid, int32_t sig)
{
    task_t *t = get_task_by_id(pid);
    if (!t || !(t->state == TASK_RUNNABLE || t->state == TASK_RUNNING)) {
        /* if a process kill itself, the process state we see is TASK_RUNNING */
        return -1;
    }

    switch (sig) {
    case SIGKILL:
        t->sig_pending |= 1 << (SIGKILL - 1);
        break;
    default:
        return -1;
    }
    return 0;
}

static inline void bit_unset(uint32_t *val, uint8_t bit)
{
    *val &= ~(1 << bit);
}

static inline void bit_set(uint32_t *val, uint8_t bit)
{
    *val |= (1 << bit);
}

static inline bool bit_test_and_set(uint32_t *val, uint8_t bit)
{
    bool t = (*val & (1 << bit));
    bit_set(val, bit);
    return t;
}

void do_signal()
{
    /* do signal handler in kernel mode */
    task_t *cur = (task_t *) get_current();
    uint32_t p = cur->sig_pending;
    while (p) {
        uint32_t idx = __builtin_ffs(p) - 1;
        bit_unset(&p, idx);
        if (idx != -1 && !bit_test_and_set(&cur->sig_blocked, idx)) {
            /* found pending and non-blocked sigal */
            bit_unset(&cur->sig_pending, idx);
            switch (idx + 1) {
            case SIGKILL:
                /* reselect process */
                do_exit();
            default:
            }
        }
    }
}