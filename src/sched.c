#include "sched.h"
#include "peripherals/uart.h"
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
        task_pool[i].task_state = TASK_UNUSED;
    }

    // initialize main() as idls task
    task_t *self = &task_pool[0];
    self->tid = 0;
    self->task_state = TASK_RUNNING;
    asm volatile("msr tpidr_el1, %0" ::"r"(self));
}

void privilege_task_create(void (*func)())
{
    // look for unused task struct
    uint32_t tid = 0;
    for (tid = 0; tid < MAX_TASK; ++tid) {
        if (task_pool[tid].task_state == TASK_UNUSED)
            break;
    }
    if (MAX_TASK == tid)
        return;

    task_t *task = &task_pool[tid];
    task->tid = tid;
    task->task_context.fp = (uint64_t) 0;  // arbitarary value
    task->task_context.sp = (uint64_t) &kstack_pool[tid + 1][0];
    task->task_context.lr = (uint64_t) *func;
    task->task_state = TASK_RUNNABLE;
    task->remain = EPOCH;

    if (!runqueue_is_full(&runqueue))
        runqueue_push(&runqueue, &task);
}

void schedule()
{
    task_t *current, *next, *idle = &task_pool[0];
    asm volatile("mrs %0, tpidr_el1" : "=r"(current));
    if (current != idle) {
        // reset if it's zero
        if (!current->remain) {
            current->remain = EPOCH;
        }
        // push current task into runqueue except idle task
        runqueue_push(&runqueue, &current);
    }
    if (runqueue_is_empty(&runqueue)) {
        next = idle;
    } else {
        runqueue_pop(&runqueue, &next);
    }
    context_switch(next);
}

static inline void reschedule()
{
    task_t *current;
    asm volatile("mrs %0, tpidr_el1" : "=r"(current));
    if (!current->remain) {
        uart_printf("task %d yield cpu...\n", current->tid);
        schedule();
    }
}

void task1()
{
    while (1) {
        uart_printf("1...\n");
        for (int i = 0; i < (1 << 26); ++i)
            asm("nop");
        reschedule();
    }
    __builtin_unreachable();
}

void task2()
{
    while (1) {
        uart_printf("2...\n");
        for (int i = 0; i < (1 << 26); ++i)
            asm("nop");
        reschedule();
    }
    __builtin_unreachable();
}

void task3()
{
    while (1) {
        uart_printf("3...\n");
        for (int i = 0; i < (1 << 26); ++i)
            asm("nop");
        reschedule();
    }
    __builtin_unreachable();
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
