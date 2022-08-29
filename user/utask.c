#include <include/stdio.h>
#include <include/types.h>
#include <include/utask.h>

void utask3()
{
    while (1) {
        printf("3...\n");
        for (int i = 0; i < (1 << 26); ++i)
            asm("nop");
    }
}