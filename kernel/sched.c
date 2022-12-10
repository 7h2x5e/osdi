#include <include/irq.h>
#include <include/sched.h>
#include <include/task.h>
#include <include/types.h>
#include <include/mm.h>

void schedule()
{
    task_t *current = (task_t *) get_current(),
           *next = get_task_by_id(0);  // idle task

    /*
     * schedule() may be called when returning to user space from irq handler
     * in this senario, irq disable bit is set to 1 and schedule() will not be
     * interrupted by irq handler so that we don't need to protect critical
     * section
     */
    uint32_t daif;
    asm volatile("mrs %0, daif" : "=r"(daif) :);
    uint8_t irq_disable_bit = (daif >> 7) & 1;

    // push current task into runqueue except idle, zombie and blocked task
    if (current != next && current->state != TASK_ZOMBIE &&
        current->state != TASK_BLOCKED) {
        if (!current->counter) {
            current->counter = TASK_EPOCH;
        }
        current->state = TASK_RUNNABLE;
        if (!irq_disable_bit)
            disable_irq();
        {
            runqueue_push(&runqueue, &current);
        }
        if (!irq_disable_bit)
            enable_irq();
    }
    // get a task from runqueue, may get the task itself
    if (!irq_disable_bit)
        disable_irq();
    {
        if (!runqueue_is_empty(&runqueue)) {
            runqueue_pop(&runqueue, &next);
        }
    }
    if (!irq_disable_bit)
        enable_irq();
    next->state = TASK_RUNNING;
    if (!irq_disable_bit)
        disable_irq();
    {
        context_switch(next);
    }
    if (!irq_disable_bit)
        enable_irq();
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
    update_pgd(next->mm.pgd);
    switch_to(prev, next);
}