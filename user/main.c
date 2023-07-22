#include "user/ulib.h"

void delay(int period)
{
    while (period--)
        ;
}

int main()
{
    int64_t count = 0;

    while (count++ < 2) {
        if (fork() == -1) {
            printf("fork failed!\n");
            break;
        }
        printf("pid=%d, count=%d\n", get_taskid(), count);
    }
    return 0;
}