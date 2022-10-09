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

void test()
{
    int cnt = 1;
    if (fork() == 0) {
        /* child process */
        fork();
        delay(100000000);
        fork();
        while (cnt < 10) {
            printf("Task id: %d, cnt: %d\n", get_taskid(), cnt);
            delay(100000000);
            ++cnt;
        }
        exit();
        printf("Should not be printed\n");
    } else {
        /* parent process */
        printf("Task %d before exec, cnt address 0x%h, cnt value %d\n",
               get_taskid(), &cnt, cnt);
        exec(foo);
    }
}
