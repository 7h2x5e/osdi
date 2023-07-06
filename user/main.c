#include "user/ulib.h"

void delay(int period)
{
    while (period--)
        ;
}

int main()
{
    void *new_addr = mmap((void *) 0xdeadbeef, 4097, PROT_READ | PROT_WRITE,
                          MAP_ANONYMOUS, NULL, 0);
    printf("allocate a page, start at 0x%x\n", new_addr);
    // TODO: munmap
    return 0;
}