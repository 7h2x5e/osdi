#ifndef _TASK_H
#define _TASK_H

#define THREAD_CPU_CONTEXT 0
#define TASK_EPOCH 1

#ifndef __ASSEMBLER__

#include <include/types.h>

/* runqueue ring buffer */
#define MAX_TASK (1 << 6)
#define RUNQUEUE_SIZE MAX_TASK  // must be power of 2

/* task pool */
#define KSTACK_SIZE (1 << 12)
#define USTACK_SIZE (1 << 12)

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

extern runqueue_t runqueue;
extern task_t task_pool[MAX_TASK];
extern uint8_t kstack_pool[MAX_TASK][KSTACK_SIZE];
extern uint8_t ustack_pool[MAX_TASK][USTACK_SIZE];

extern task_t *get_current();
void init_task();
void do_exec(void (*)());
void privilege_task_create(void (*)());
void task1();
void task2();
void task3();

#endif
#endif
