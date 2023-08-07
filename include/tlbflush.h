#ifndef _TLBFLUSH_H
#define _TLBFLUSH_H

static inline void flush_tlb_all()
{
    asm volatile(
        "dsb ish\n"
        "tlbi vmalle1is\n"
        "dsb ish\n"
        "isb");
}

#endif
