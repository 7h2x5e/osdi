#include "time.h"
#include "sys.h"
#include "types.h"

float get_timestamp()
{
    /* wrapper of sys_timestamp */
    uint64_t freq, cnt;
    register uint64_t *t0 __asm__("x0") = &freq, *t1 __asm__("x1") = &cnt;
    asm volatile("mov x8," xstr(SYS_TIMESTAMP_NUMBER) "\n"
                                                    "svc #0");
    return (float) cnt / freq;
}