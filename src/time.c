#include "time.h"

float getTime() {
  register unsigned long f, t;
  // Get the current counter frequency
  asm volatile("mrs %0, cntfrq_el0" : "=r"(f));
  // Read the current counter
  asm volatile("mrs %0, cntpct_el0" : "=r"(t));
  return (float)t / f;
}