#include <include/irq.h>
#include <include/sched.h>
#include <include/task.h>
#include <include/types.h>

void schedule()
{
    task_t *current = (task_t *) get_current(),
           *next = &task_pool[0];  // idle task
    if (current != next) {
        // push current task into runqueue except idle task
        if (!current->counter) {
            current->counter = TASK_EPOCH;
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
    }
}

void context_switch(task_t *next)
{
    task_t *prev = (task_t *) get_current();
    switch_to(prev, next);
}