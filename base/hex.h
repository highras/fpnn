#ifndef hex_h_
#define hex_h_

#ifdef __cplusplus
extern "C" {
#endif


/* Return the number of character encoded in 'dst', not including the 
 * tailing '\0'. The encoded string in 'dst' is lower-cased.
 */
int hexlify(char *dst, const void *src, int size);


/* This function is the same as hexlify() except its encoded string 
 * in 'dst' is upper-cased.
 */
int Hexlify(char *dst, const void *src, int size);


/* Return the number of bytes decoded into 'dst'.
 * On error, return a negative number, whose absolute value equals to
 * the consumed number of character in the 'src'.
 * If 'size' is -1, size is taken as strlen(src).
 */
int unhexlify(void *dst, const char *src, int size);


#ifdef __cplusplus
}
#endif

#endif
