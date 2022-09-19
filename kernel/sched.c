#include <include/irq.h>
#include <include/sched.h>
#include <include/task.h>
#include <include/types.h>

void schedule()
{
    task_t *current = (task_t *) get_current(),
           *next = get_task_by_id(0);  // idle task
    // push current task into runqueue except idle task or zombie task
    if (current != next && current->state != TASK_ZOMBIE) {
        if (!current->counter) {
            current->counter = TASK_EPOCH;
        }
        current->state = TASK_RUNNABLE;
        runqueue_push(&runqueue, &current);
    }
    // get a task from runqueue, may get the task itself
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