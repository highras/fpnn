#ifndef RDTSC_H_
#define RDTSC_H_ 1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __arm__

extern uint64_t xs_cpu_frequency;


/* Read the Time Stamp Counter.
 */
uint64_t rdtsc();


/* This function will update extern variable xs_cpu_frequency. 
 */
uint64_t get_cpu_frequency(unsigned int milliseconds);


uint64_t cpu_frequency();

#endif


#ifdef __cplusplus
}
#endif

#endif
