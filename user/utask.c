#include <include/stdio.h>
#include <include/syscall.h>
#include <include/types.h>
#include <include/utask.h>

void utask3_exec()
{
    while (1) {
        printf("[PID %d] fork...\n", get_taskid());
        int64_t tid = fork();
        if (tid < 0) {
            printf("failed!\n");
            continue;
        }
        if (!tid) {
            /* child process */
            printf("[PID %d] child process!\n", get_taskid());
        } else {
            /* parent process */
            printf("[PID %d] parent process!\n", get_taskid());
            printf("[PID %d] exit...\n", get_taskid());
            exit();
        }
        for (int i = 0; i < (1 << 26); ++i)
            asm("nop");
    }
}

void utask3()
{
    printf("[PID %d] fork...\n", get_taskid());
    exec(&utask3_exec);
    __builtin_unreachable();
}