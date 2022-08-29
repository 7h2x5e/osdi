#ifndef _SCHED_H
#define _SCHED_H

#include <include/task.h>

extern void switch_to(task_t *, task_t *);
void schedule();
void reschedule();
void context_switch(task_t *next);

#endif
