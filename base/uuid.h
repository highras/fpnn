#ifndef uuid_h_
#define uuid_h_

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE	1	//-- for compiling warning in /usr/include/features.h.
#endif

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


#define UUID_STRING_LEN		36


typedef unsigned char uuid_t[16];


void uuid_generate(uuid_t uuid);

void uuid_generate_random(uuid_t uuid);

void uuid_generate_time(uuid_t uuid);
 

size_t uuid_string(const uuid_t uuid, char buf[], size_t len);


void uuid_get_random_bytes(void *buf, int nbytes);


#ifdef __cplusplus
}
#endif

#endif
