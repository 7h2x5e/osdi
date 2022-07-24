#ifndef SYS_H
#define SYS_H

#define xstr(s) str(s)
#define str(s) #s
#define __NR_syscalls 4

#define SYS_TOGGLE_IRQ_NUMBER 0
#define SYS_TIMESTAMP_NUMBER 1

#endif
