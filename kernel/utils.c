#include <include/types.h>
#include <include/utils.h>

#define PM_PASSWORD 0x5a000000
#define PM_RSTC ((volatile unsigned int *) (0x3F10001c))
#define PM_WDOG ((volatile unsigned int *) (0x3F100024))

int64_t do_reset(uint64_t tick)
{
    // reboot after watchdog timer expire
    *PM_RSTC = PM_PASSWORD | 0x20;  // full reset
    *PM_WDOG = PM_PASSWORD | tick;  // number of watchdog tick
    return 0;
}

int64_t do_cancel_reset()
{
    *PM_RSTC = PM_PASSWORD | 0;  // full reset
    *PM_WDOG = PM_PASSWORD | 0;  // number of watchdog tick
    return 0;
}

int64_t do_get_timestamp(struct TimeStamp *ts)
{
    asm volatile(
        "mrs %[CNTFRQ], cntfrq_el0\n"
        "mrs %[CNTPCT], cntpct_el0"
        : [CNTFRQ] "=r"(ts->freq), [CNTPCT] "=r"(ts->counts)
        :);
    return 0;
}