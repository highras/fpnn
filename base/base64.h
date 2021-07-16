#ifndef BASE64_H_
#define BASE64_H_ 1

#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BASE64_LEN(n)	(((n) * 4 + 2) / 3)

typedef struct base64_t base64_t;

struct base64_t {
	unsigned char alphabet[66];
	unsigned char detab[256];
};

/*  ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=  */
extern const base64_t std_base64;

/*  ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_=  */
extern const base64_t url_base64;


enum {
	BASE64_NO_PADDING = 0x01,
	BASE64_AUTO_NEWLINE = 0x02,
	BASE64_IGNORE_SPACE = 0x10,
	BASE64_IGNORE_NON_ALPHABET = 0x20,
};


/* The alphabet[64] used as padding character if not '\0'.
 * Return 0 if success, otherwise -1.
 */
int base64_init(base64_t *b64, const char *alphabet);


/* Return the number of characters placed in out. 
 */
ssize_t base64_encode(const base64_t *b64, char *out, const void *in, size_t len, int flag);

 
/* Return the number of characters placed in out. 
 * On error, return a negative number, the absolute value of the number
 * equals to the consumed size of the input string.
 */
ssize_t base64_decode(const base64_t *b64, void *out, const char *in, size_t len, int flag);


#ifdef __cplusplus
}
#endif


#endif

