#ifndef MSEC_H_
#define MSEC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Fast but less precision
 */
int64_t slack_mono_msec();
int64_t slack_real_msec();

/* For second, Fast but less precision
 * */
int64_t slack_mono_sec();
int64_t slack_real_sec();


/* Slow but accurate
 */
//millisecond
int64_t exact_mono_msec();
int64_t exact_real_msec();

//microsecond
int64_t exact_mono_usec();
int64_t exact_real_usec();

//nsecond
int64_t exact_mono_nsec();
int64_t exact_real_nsec();

#ifdef __cplusplus
}
#endif

#endif
