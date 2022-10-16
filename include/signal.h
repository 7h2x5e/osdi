#ifndef SIGNAL_H
#define SIGNAL_H

#include <include/task.h>
#include <include/types.h>

/* This part is taken from include/uapi/asm-generic/signal.h */
#define SIGKILL 1
#define SIGUNUSED 2

typedef void (*sig_t)(int32_t);

#define SIG_DFL ((sig_t) 0)  /* default signal handling */
#define SIG_IGN ((sig_t) 1)  /* ignore signal */
#define SIG_ERR ((sig_t) -1) /* error return from signal */

void do_signal();
int32_t do_kill(pid_t pid, int32_t sig);

#endif