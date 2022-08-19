#ifndef _SCHED_H
#define _SCHED_H

#define THREAD_CPU_CONTEXT 0
#define MAX_TASK (1 << 6)
#define KSTACK_SIZE (1 << 12)
#define USTACK_SIZE (1 << 12)

#ifndef __ASSEMBLER__

#include "types.h"

typedef enum { TASK_UNUSED, TASK_RUNNABLE, TASK_RUNNING } task_state;

struct task_context {
    uint64_t x19;
    uint64_t x20;
    uint64_t x21;
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28;
    uint64_t fp;
    uint64_t lr;
    uint64_t sp;
};

typedef struct task_struct {
    struct task_context task_context;
    uint64_t tid;
    task_state state;
    uint64_t counter;
} task_t;

extern task_t *get_current();
extern void switch_to(task_t *, task_t *);
extern void enable_irq();
extern void disable_irq();
void init_task();
void do_exec(void (*)());
void privilege_task_create(void (*)());
void context_switch(task_t *);
void schedule();
void reschedule();

#endif
#endif
