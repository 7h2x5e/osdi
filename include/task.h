#ifndef _TASK_H
#define _TASK_H

#define THREAD_CPU_CONTEXT 0
#define TASK_EPOCH 1

#ifndef __ASSEMBLER__

#include <include/exc.h>
#include <include/list.h>
#include <include/types.h>
#include <include/mm.h>

/* runqueue ring buffer */
#define MAX_TASK (1 << 6)
#define RUNQUEUE_SIZE MAX_TASK  // must be power of 2

/* task pool */
#define KSTACK_SIZE (1 << 12)
#define USTACK_SIZE (1 << 12)

typedef enum {
    TASK_UNUSED,
    TASK_RUNNABLE,
    TASK_RUNNING,
    TASK_ZOMBIE,
    TASK_BLOCKED
} task_state;

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

typedef uint32_t pid_t;
typedef uint32_t sigvec_t;

typedef struct task_struct {
    struct task_context task_context;
    pid_t tid;
    task_state state;
    uint64_t counter;
    sigvec_t sig_pending;
    sigvec_t sig_blocked;
    mm_struct mm;
    struct list_head node;
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

extern runqueue_t runqueue, waitqueue;
extern struct list_head zombie_list;

extern const task_t *get_current();
task_t *get_task_by_id(uint32_t);
void init_task();
void *get_kstack_by_id(uint32_t);
void *get_ustack_by_id(uint32_t);
void *get_kstacktop_by_id(uint32_t);
void *get_ustacktop_by_id(uint32_t);
uint32_t do_get_taskid();
void do_exec(void (*)(), size_t, uint64_t);
int64_t do_fork(struct TrapFrame *);
void do_exit();
int64_t privilege_task_create(void (*)());
void idle();
void zombie_reaper();
void update_pgd(uintptr_t);

#endif
#endif
