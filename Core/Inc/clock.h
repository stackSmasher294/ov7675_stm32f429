#ifndef __CLOCK_H
#define __CLOCK_H
#include "main.h"

void init_perf_counter();
void reset_perf_counter();
uint32_t clock_begin();
uint32_t clock_end();


#endif