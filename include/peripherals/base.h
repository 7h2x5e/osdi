#ifndef BASE_H
#define BASE_H

#include <include/mm.h>

#define MMIO_BASE (KERNEL_VIRT_BASE | 0x3F000000)
#define LOCAL_PERIPHERAL_BASE (KERNEL_VIRT_BASE | 0x40000000)

#endif