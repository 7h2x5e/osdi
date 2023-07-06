#include "user/ulib.h"

void delay(int period)
{
    while (period--)
        ;
}

int main()
{
    void *addr;
    if (MAP_FAILED != (uint64_t) (addr = mmap((void *) 0xdeadbeef, 4097,
                                              PROT_READ | PROT_WRITE,
                                              MAP_ANONYMOUS, NULL, 0))) {
        printf("[user] mmap a new region, start at 0x%x\n", addr);
        // munmap(addr, 8193);
    }

    int64_t pid = fork();
    if (pid == -1) {
        printf("[user] fork failed\n");
    } else if (pid) {
        printf("[user] parent process, get child process pid = %d\n", pid);
    } else {
        printf("[user] child process, pid = %d\n", pid);
    }

    return 0;
}