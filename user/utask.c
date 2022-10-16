#include <include/signal.h>
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

__attribute__((optimize("O0"))) static void delay(uint64_t count)
{
    for (uint64_t i = 0; i < count; ++i)
        asm("nop");
}

void foo()
{
    int tmp = 5;
    printf("Task %d after exec, tmp address 0x%h, tmp value %d\n", get_taskid(),
           &tmp, tmp);
    exit();
}

void utest1()
{
    char buf[256];
    int n;
    do {
        printf("[PID %d] Read up to 1 bytes...\n", get_taskid());
        delay(1000000000);
        n = uart_read(buf, 1);
    } while (n < 1);
    printf("Will not reach here if the process is killed by signal\n", n);
    exit();
}

void utest2()
{
    int success;
    do {
        printf("[PID %d] Send signal to kill pid 2 process... %s!\n",
               get_taskid(),
               (success = kill(2, SIGKILL)) == 0 ? "succeed"
                                                 : "process do not exist");
        delay(1000000000);
    } while (!success);
    exit();
}
