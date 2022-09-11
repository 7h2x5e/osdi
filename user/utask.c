#include <include/stdio.h>
#include <include/syscall.h>
#include <include/types.h>
#include <include/utask.h>

void utask3_exec()
{
    while (1) {
        printf("3...\n");
        for (int i = 0; i < (1 << 26); ++i)
            asm("nop");
    }
}

void utask3()
{
    printf("exec...\n");
    exec(&utask3_exec);
    __builtin_unreachable();
}