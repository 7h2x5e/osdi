#include <include/arm/sysregs.h>
#include <include/irq.h>
#include <include/printk.h>
#include <include/sched.h>
#include <include/string.h>
#include <include/task.h>
#include <include/types.h>
#include <include/utask.h>

runqueue_t runqueue, waitqueue;
struct list_head zombie_list;
static task_t task_pool[MAX_TASK] __attribute__((section(".kstack")));
static uint8_t kstack_pool[MAX_TASK][KSTACK_SIZE]
    __attribute__((section(".kstack")));
static uint8_t ustack_pool[MAX_TASK][USTACK_SIZE]
    __attribute__((section(".ustack")));

/*
 * Return a pointer pointing to a task no matter what the task's state is.
 */
task_t *get_task_by_id(uint32_t id)
{
    if (id < MAX_TASK) {
        return &task_pool[id];
    }
    return NULL;
}

uint32_t do_get_taskid()
{
    const task_t *task = get_current();
    return task->tid;
}

void *get_kstack_by_id(uint32_t id)
{
    return (void *) &kstack_pool[id][0];
}

void *get_ustack_by_id(uint32_t id)
{
    return (void *) &ustack_pool[id][0];
}

void *get_kstacktop_by_id(uint32_t id)
{
    return (void *) &kstack_pool[id + 1][0];
}

void *get_ustacktop_by_id(uint32_t id)
{
    return (void *) &ustack_pool[id + 1][0];
}

void init_task()
{
    runqueue_init(&runqueue);
    runqueue_init(&waitqueue);
    INIT_LIST_HEAD(&zombie_list);

    for (uint32_t i = 0; i < MAX_TASK; ++i) {
        task_pool[i].state = TASK_UNUSED;
        INIT_LIST_HEAD(&task_pool[i].node);
    }

    // initialize main() as idle task
    task_t *self = &task_pool[0];
    self->tid = 0;
    self->state = TASK_RUNNING;
    asm volatile("msr tpidr_el1, %0" ::"r"(self));
}

static inline int64_t get_pid()
{
    // look for unused task struct
    uint32_t tid = 0;
    for (tid = 0; tid < MAX_TASK; ++tid) {
        if (task_pool[tid].state == TASK_UNUSED)
            return tid;
    }
    return -1;
}

void do_exec(void (*func)())
{
    task_t *task = (task_t *) get_current();
    // stack address grows towards lower memory address, so we use address of
    // top of stack of next user task as initial stack address.
    void *ustack = get_ustacktop_by_id(task->tid);

    // switch to el0
    asm volatile(
        "msr     sp_el0, %0\n\t"
        "msr     elr_el1, %1\n\t"
        "msr     spsr_el1, %2\n\t"
        "eret" ::"r"(ustack),
        "r"(func), "r"(SPSR_EL1_VALUE));
    __builtin_unreachable();
}

/*
 * On success, the task id of the child process is returned
 * in the parent, and 0 is returned in the child. On failure,
 * -1 is returned in the parent.
 */
int64_t do_fork(struct TrapFrame *tf)
{
    // create a new task: child
    int64_t new_task_id = privilege_task_create(NULL);
    if (new_task_id < 0)
        return -1;
    task_t *new_task = get_task_by_id(new_task_id);
    const task_t *cur_task = get_current();

    void *ustack_cur = get_ustack_by_id(cur_task->tid);
    void *ustack_new = get_ustack_by_id(new_task->tid);
    void *ustacktop_cur = get_ustacktop_by_id(cur_task->tid);
    void *ustacktop_new = get_ustacktop_by_id(new_task->tid);
    void *kstacktop_new = get_kstacktop_by_id(new_task->tid);

    // copy user context
    memcpy(ustack_new, ustack_cur, USTACK_SIZE);

    // set child's trapframe
    struct TrapFrame *tf_new =
        (struct TrapFrame *) (kstacktop_new - sizeof(struct TrapFrame));
    *tf_new = *tf;

    // set child's return value
    tf_new->x[0] = 0;

    // change child's user stack
    tf_new->sp = (uint64_t) (ustacktop_new + ((void *) tf->sp - ustacktop_cur));

    // set child task's task_struct
    extern void ret_to_user();
    new_task->task_context.lr = (uint64_t) ret_to_user;
    new_task->task_context.sp = (uint64_t) tf_new;
    new_task->state = TASK_RUNNABLE;
    new_task->sig_blocked = cur_task->sig_blocked;
    new_task->sig_pending = cur_task->sig_pending;

    return new_task_id;
}

void do_exit()
{
    task_t *cur = (task_t *) get_current();
    cur->state = TASK_ZOMBIE;
    list_add_tail(&cur->node, &zombie_list);
    schedule();
    __builtin_unreachable();
}

int64_t privilege_task_create(void (*func)())
{
    int64_t tid = get_pid();
    if (tid < 0 || runqueue_is_full(&runqueue))
        return -1;

    task_t *task = &task_pool[tid];
    task->tid = tid;
    task->task_context.sp = (uint64_t) get_kstacktop_by_id(tid);
    task->task_context.lr = (uint64_t) *func;
    task->state = TASK_RUNNABLE;
    task->counter = TASK_EPOCH;
    task->sig_pending = 0;
    task->sig_blocked = 0;
    runqueue_push(&runqueue, &task);

    return (int64_t) tid;
}

void runqueue_init(runqueue_t *rq)
{
    rq->size = RUNQUEUE_SIZE;
    rq->mask = rq->size - 1;
    runqueue_reset(rq);
}

void runqueue_reset(runqueue_t *rq)
{
    rq->head = rq->tail = 0;
}

size_t runqueue_size(const runqueue_t *rq)
{
    return rq->size;
}

bool runqueue_is_full(const runqueue_t *rq)
{
    __sync_synchronize();
    return (rq->head == ((rq->tail + 1) & rq->mask));
}

bool runqueue_is_empty(const runqueue_t *rq)
{
    __sync_synchronize();
    return rq->head == rq->tail;
}

void runqueue_push(runqueue_t *rq, task_t **t)
{
    // assume buffer isn't full
    rq->buf[rq->tail++] = *t;
    rq->tail &= rq->mask;
}

void runqueue_pop(runqueue_t *rq, task_t **t)
{
    // assume buffer isn't empty
    *t = rq->buf[rq->head++];
    rq->head &= rq->mask;
}

__attribute__((optimize("O0"))) static void delay(uint64_t count)
{
    for (uint64_t i = 0; i < count; ++i)
        asm("nop");
}

void idle()
{
    while (1) {
        if (runqueue_is_empty(&runqueue)) {
            break;
        }
        reschedule();
        delay(100000000);
    }
    printk("Test finished\n");
    while (1)
        ;
}

void zombie_reaper()
{
    while (1) {
        if (list_empty(&zombie_list)) {
            schedule();
            enable_irq();
        } else {
            /* free zombie task's resource */
            task_t *zt, *tmp;
            list_for_each_entry_safe(zt, tmp, &zombie_list, node)
            {
                list_del_init(&zt->node);
                zt->state = TASK_UNUSED;
                printk(
                    "[PID %d] Zombie reaper frees process [PID %d] resources\n",
                    do_get_taskid(), zt->tid);
            }
        }
    }
}