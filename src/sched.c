#include "sched.h"
#include "asm/sysregs.h"
#include "types.h"

/* runqueue ring buffer */
#define RUNQUEUE_SIZE MAX_TASK  // must be power of 2
typedef struct runqueue_t {
    task_t *buf[RUNQUEUE_SIZE];
    size_t head, tail;
    size_t size, mask;
} runqueue_t;

void runqueue_init(runqueue_t *);
void runqueue_reset(runqueue_t *);
size_t runqueue_size(const runqueue_t *);
bool runqueue_is_full(const runqueue_t *);
bool runqueue_is_empty(const runqueue_t *);
void runqueue_push(runqueue_t *, task_t **);
void runqueue_pop(runqueue_t *, task_t **);

static runqueue_t runqueue;
static task_t task_pool[MAX_TASK] __attribute__((section(".kstack")));
static uint8_t kstack_pool[MAX_TASK][KSTACK_SIZE]
    __attribute__((section(".kstack")));
static uint8_t ustack_pool[MAX_TASK][USTACK_SIZE]
    __attribute__((section(".ustack")));

#define EPOCH 2

void context_switch(task_t *next)
{
    task_t *prev = (task_t *) get_current();
    switch_to(prev, next);
}

void init_task()
{
    runqueue_init(&runqueue);

    for (uint32_t i = 0; i < MAX_TASK; ++i) {
        task_pool[i].state = TASK_UNUSED;
    }

    // initialize main() as idls task
    task_t *self = &task_pool[0];
    self->tid = 0;
    self->state = TASK_RUNNING;
    asm volatile("msr tpidr_el1, %0" ::"r"(self));
}

static inline int32_t get_pid()
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
    void *ustack = &ustack_pool[task->tid + 1][0];

    // switch to el0
    asm volatile(
        "msr     sp_el0, %0\n\t"
        "msr     elr_el1, %1\n\t"
        "msr     spsr_el1, %2\n\t"
        "eret" ::"r"(ustack),
        "r"(func), "r"(SPSR_EL1_VALUE));
    __builtin_unreachable();
}

void privilege_task_create(void (*func)())
{
    int32_t tid = get_pid();
    if (tid < 0)
        return;

    task_t *task = &task_pool[tid];
    task->tid = tid;
    task->task_context.sp = (uint64_t) &kstack_pool[tid + 1][0];
    task->task_context.lr = (uint64_t) *func;
    task->state = TASK_RUNNABLE;
    task->counter = EPOCH;

    if (!runqueue_is_full(&runqueue))
        runqueue_push(&runqueue, &task);
}

void schedule()
{
    task_t *current = (task_t *) get_current(),
           *next = &task_pool[0];  // idle task
    if (current != next) {
        // push current task into runqueue except idle task
        if (!current->counter) {
            current->counter = EPOCH;
        }
        current->state = TASK_RUNNABLE;
        runqueue_push(&runqueue, &current);
    }
    if (!runqueue_is_empty(&runqueue)) {
        runqueue_pop(&runqueue, &next);
    }
    next->state = TASK_RUNNING;
    context_switch(next);
}

void reschedule()
{
    task_t *current = (task_t *) get_current();
    if (0 >= current->counter) {
        schedule();
        enable_irq();
    }
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
    return (rq->head == (rq->tail + 1)) & rq->mask;
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
