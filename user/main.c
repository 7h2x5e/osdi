#include "user/ulib.h"

void delay(int period)
{
    while (period--)
        ;
}

int main()
{
    int count = 10;
    while (count) {
        printf("[PID %d] Hello world x%d\n", get_taskid(), count--);
        delay(10000000);
    }
    return 0;
}