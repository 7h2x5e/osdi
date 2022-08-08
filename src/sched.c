#include "sched.h"
#include "peripherals/uart.h"
#include "types.h"

static task_t task_pool[MAX_TASK] __attribute__((section(".kstack")));
static uint8_t kstack_pool[MAX_TASK][KSTACK_SIZE]
    __attribute__((section(".kstack")));
static int task_id = 1;  // id = 0 is preserved for main()

static inline int get_task_id()
{
    int t = task_id;
    task_id = (task_id + 1) % MAX_TASK;
    return t;
}

task_t *get_task(int tid)
{
    return &task_pool[tid];
}

void privilege_task_create(void (*func)())
{
    int id = get_task_id();
    task_t *task = &task_pool[id];
    task->id = id;
    task->task_context.fp = (uint64_t) 0;  // arbitarary value
    task->task_context.sp = (uint64_t) &kstack_pool[id + 1][0];
    task->task_context.lr = (uint64_t) *func;
}

void context_switch(task_t *next)
{
    task_t *prev = (task_t *) get_current();
    switch_to(prev, next);
}

void task1()
{
    while (1) {
        uart_printf("1...\n");
        for (int i = 0; i < (1 << 26); ++i)
            asm("nop");
        context_switch(&task_pool[2]);
    }
    __builtin_unreachable();
}

void task2()
{
    while (1) {
        uart_printf("2...\n");
        for (int i = 0; i < (1 << 26); ++i)
            asm("nop");
        context_switch(&task_pool[1]);
    }
    __builtin_unreachable();
}