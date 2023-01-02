#ifndef EXC_H
#define EXC_H

#include <include/types.h>

// ESR_EL1, Exception Syndrome Register (EL1)
#define ESR_ELx_ISS_SHIFT 0
#define ESR_ELx_IL_SHIFT 25
#define ESR_ELx_EC_SHIFT 26
#define ESR_ELx_EC_SVC64 0x15
#define ESR_ELx_EC_INST_ABORT_LOW 0x20
#define ESR_ELx_EC_DATA_ABORT_LOW 0x24

struct TrapFrame {
    uint64_t x[31];  // x0-x30
    uint64_t sp, type, esr_el1, elr_el1, spsr_el1;
};

void show_invalid_entry_message(struct TrapFrame *);
void sync_handler(struct TrapFrame *);
void svc_handler(struct TrapFrame *);

#endif