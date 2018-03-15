#ifndef RIJNDAEL_H_
#define RIJNDAEL_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
	int nrounds;
	uint32_t rk[60];
} rijndael_context;


/* The 'keylen' must be 16, 24, or 32.
 */
bool rijndael_setup_encrypt(rijndael_context *ctx, const uint8_t *key, size_t keylen);
bool rijndael_setup_decrypt(rijndael_context *ctx, const uint8_t *key, size_t keylen);


/*
   NB: In following encryption and decryption functions, 
   the input and output buffers can be the same.
   When input and output buffers are the same, the encryption or decryption
   is done in place.
 */


void rijndael_encrypt(const rijndael_context *ctx, const uint8_t plain[16], uint8_t cipher[16]);
void rijndael_decrypt(const rijndael_context *ctx, const uint8_t cipher[16], uint8_t plain[16]);


/*
   The argument 'len' is the length of the 'plain' buffer.
   The actual size of 'cipher' must always be multiple of 16 (greater than
   or equal to 'len').
   If the 'len' is not multiple of 16, '\0' is padded when encryption, and 
   extra bytes is discarded when decryption.
 */
void rijndael_cbc_encrypt(const rijndael_context *ctx, const uint8_t *plain, uint8_t *cipher, size_t len, uint8_t ivec[16]);
void rijndael_cbc_decrypt(const rijndael_context *ctx, const uint8_t *cipher, uint8_t *plain, size_t len, uint8_t ivec[16]);


/*
   The first argument 'ctx' should be set by rijndael_setup_encrypt().
   When encryption, set the second argument 'encrypt' to true.
   If decrypt, set the second argument 'encrypt' to false.
 */
void rijndael_cfb_encrypt(const rijndael_context *ctx, bool encrypt, const uint8_t *in, uint8_t *out, size_t len, uint8_t ivec[16], size_t *p_num);


/*
   The first argument 'ctx' should be set by rijndael_setup_encrypt().
   Encryption and decryption both use this function.
 */
void rijndael_ofb_encrypt(const rijndael_context *ctx, const uint8_t *in, uint8_t *out, size_t len, uint8_t ivec[16], size_t *p_num);



#ifdef __cplusplus
}
#endif

#endif

