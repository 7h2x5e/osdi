#include <include/signal.h>
#include <include/stdio.h>
#include <include/syscall.h>
#include <include/types.h>
#include <include/utask.h>

void utask1()
{
    char buf[256];
    do {
        printf("[PID %d] input string:\n", get_taskid());
        if (!fgets(buf, 256))
            continue;
        printf("[PID %d] your string: %s\n", get_taskid(), buf);
    } while (1);
    exit();
}
