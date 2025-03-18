#include "clock.h"

// #include "core_cm4.h"

void init_perf_counter()
{
    // Enable DWT and cycle counting
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t clock_begin()
{
	// Reset the cycle counter
	DWT->CYCCNT = 0;
    return DWT->CYCCNT;
}

uint32_t clock_end()
{
    return DWT->CYCCNT;
}