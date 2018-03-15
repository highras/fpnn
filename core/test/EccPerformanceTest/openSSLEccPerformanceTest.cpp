// g++ -o openSSLEccPerformanceTest openSSLEccPerformanceTest.cpp -lcrypto // -lssl -lcrypto

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <iostream>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ec.h>
#include <openssl/ecdh.h>

uint8_t ass[32];

void timeCalc(struct timeval start, struct timeval finish)
{
	int sec = finish.tv_sec - start.tv_sec;
	int usec;
	
	if (finish.tv_usec >= start.tv_usec)
		usec = finish.tv_usec - start.tv_usec;
	else
	{
		usec = 100 * 10000 + finish.tv_usec - start.tv_usec;
		sec -= 1;
	}
	
	std::cout<<"time cost "<< (sec * 1000 + usec / 1000) << "."<<(usec % 1000)<<" ms"<<std::endl;
}

void printMemory(const void* memory, size_t size)
{
	char buf[8];
	const char* data = (char*)memory;
	size_t unit_size = sizeof(void*);
	size_t range = size % unit_size;
	
	if (range)
		range = size - range + unit_size;
	else
		range = size;

	for (size_t index = 0, column = 0; index < range; index++, column++)
	{
		if (column == unit_size)
		{
			printf("    %.*s\n", (int)unit_size, buf);
			column = 0;
		}

		if (column == 0)
			printf("        ");

		char c = ' ';
		if (index < size)
		{
			printf("%02x ", (uint8_t)data[index]);
			
			c = data[index];
			if (c < '!' || c > '~')
				c = '.';
		}
		else
			printf("   ");
		buf[column] = c;
	}

	if (size)
		printf("    %.*s\n", (int)unit_size, buf);
}

void handleErrors()
{
	ERR_print_errors_fp(stderr);
}

#if 0
unsigned char *ecdh(size_t *secret_len)
{
	EVP_PKEY_CTX *pctx, *kctx;
	EVP_PKEY_CTX *ctx;
	unsigned char *secret;
	EVP_PKEY *pkey = NULL, *peerkey, *params = NULL;
	/* NB: assumes pkey, peerkey have been already set up */

	/* Create the context for parameter generation */
	if(NULL == (pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL))) handleErrors();

	/* Initialise the parameter generation */
	if(1 != EVP_PKEY_paramgen_init(pctx)) handleErrors();

	/* We're going to use the ANSI X9.62 Prime 256v1 curve */
	if(1 != EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, NID_X9_62_prime256v1)) handleErrors();

	/* Create the parameter object params */
	if (!EVP_PKEY_paramgen(pctx, &params)) handleErrors();

	/* Create the context for the key generation */
	if(NULL == (kctx = EVP_PKEY_CTX_new(params, NULL))) handleErrors();

	/* Generate the key */
	if(1 != EVP_PKEY_keygen_init(kctx)) handleErrors();
	if (1 != EVP_PKEY_keygen(kctx, &pkey)) handleErrors();

	/* Get the peer's public key, and provide the peer with our public key -
	 * how this is done will be specific to your circumstances */
	peerkey = get_peerkey(pkey);

	/* Create the context for the shared secret derivation */
	if(NULL == (ctx = EVP_PKEY_CTX_new(pkey, NULL))) handleErrors();

	/* Initialise */
	if(1 != EVP_PKEY_derive_init(ctx)) handleErrors();

	/* Provide the peer public key */
	if(1 != EVP_PKEY_derive_set_peer(ctx, peerkey)) handleErrors();

	/* Determine buffer length for shared secret */
	if(1 != EVP_PKEY_derive(ctx, NULL, secret_len)) handleErrors();

	/* Create the buffer */
	if(NULL == (secret = OPENSSL_malloc(*secret_len))) handleErrors();

	/* Derive the shared secret */
	if(1 != (EVP_PKEY_derive(ctx, secret, secret_len))) handleErrors();

	EVP_PKEY_CTX_free(ctx);
	EVP_PKEY_free(peerkey);
	EVP_PKEY_free(pkey);
	EVP_PKEY_CTX_free(kctx);
	EVP_PKEY_free(params);
	EVP_PKEY_CTX_free(pctx);

	/* Never use a derived secret directly. Typically it is passed
	 * through some hash function to produce a key */
	return secret;
}
#endif

unsigned char *ecdh_low(size_t *secret_len)
{
	EC_KEY *key, *peerkey;
	int field_size;
	unsigned char *secret;

	/* Create an Elliptic Curve Key object and set it up to use the ANSI X9.62 Prime 256v1 curve */
	if(NULL == (key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1))) handleErrors();

	/* Generate the private and public key */
	if(1 != EC_KEY_generate_key(key)) handleErrors();

	/* Get the peer's public key, and provide the peer with our public key -
	 * how this is done will be specific to your circumstances */
	//peerkey = get_peerkey_low(key);

	if(NULL == (peerkey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1))) handleErrors();
	if(1 != EC_KEY_generate_key(peerkey)) handleErrors();

	/* Calculate the size of the buffer for the shared secret */
	field_size = EC_GROUP_get_degree(EC_KEY_get0_group(key));
	*secret_len = (field_size+7)/8;

	/* Allocate the memory for the shared secret */
	if(NULL == (secret = (unsigned char*)OPENSSL_malloc(*secret_len))) handleErrors();

	/* Derive the shared secret */
	*secret_len = ECDH_compute_key(secret, *secret_len, EC_KEY_get0_public_key(peerkey),
						key, NULL);

	/* Clean up */
	EC_KEY_free(key);
	EC_KEY_free(peerkey);

	if(*secret_len <= 0)
	{
		OPENSSL_free(secret);
		return NULL;
	}

	return secret;
}

int test1()
{
	size_t secret_len;
	unsigned char * secret = ecdh_low(&secret_len);
	printf("size %lu\n", secret_len);
	OPENSSL_free(secret);
}

void test()
{
	EC_KEY *key, *peerkey;
	int field_size;
	unsigned char *secret;

	/* Create an Elliptic Curve Key object and set it up to use the ANSI X9.62 Prime 256v1 curve */
	if(NULL == (key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1))) handleErrors();

	/* Generate the private and public key */
	if(1 != EC_KEY_generate_key(key)) handleErrors();


	if(NULL == (peerkey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1))) handleErrors();
	if(1 != EC_KEY_generate_key(peerkey)) handleErrors();

	/* Calculate the size of the buffer for the shared secret */
	field_size = EC_GROUP_get_degree(EC_KEY_get0_group(key));
	size_t secret_len = (field_size+7)/8;

	/* Allocate the memory for the shared secret */
	if(NULL == (secret = (unsigned char*)OPENSSL_malloc(secret_len))) handleErrors();

	/* Derive the shared secret */
	secret_len = ECDH_compute_key(secret, secret_len, EC_KEY_get0_public_key(peerkey),
						key, NULL);


	int field_size2;
	unsigned char *secret2;

	field_size2 = EC_GROUP_get_degree(EC_KEY_get0_group(peerkey));
	size_t secret_len2 = (field_size2+7)/8;

	/* Allocate the memory for the shared secret */
	if(NULL == (secret2 = (unsigned char*)OPENSSL_malloc(secret_len2))) handleErrors();

	/* Derive the shared secret */
	secret_len2 = ECDH_compute_key(secret2, secret_len2, EC_KEY_get0_public_key(key),
						peerkey, NULL);

	if (memcmp(secret, secret2, 32) != 0)
	{
		printf("memcmp failed.\n");
	}
	else
		for (int i = 0; i < 32; i++)
			ass[i] &= secret[i];

	/* Clean up */
	EC_KEY_free(key);
	EC_KEY_free(peerkey);

	OPENSSL_free(secret);
	OPENSSL_free(secret2);
}

int main(int argc, const char* argv[])
{
	for (int i = 0; i < 32; i++)
		ass[i] = 0;

	int count = 10000;
	if (argc > 1)
		count = atoi(argv[1]);

	std::cout<<"times: "<<count<<std::endl;

	//----------------------[ OpenSSL: Begin ]--------------------------//
	/* Load the human readable error strings for libcrypto */
	ERR_load_crypto_strings();

	/* Load all digest and cipher algorithms */
	OpenSSL_add_all_algorithms();

	/* Load config file, and other important initialisation */
	OPENSSL_config(NULL);
	//----------------------[ OpenSSL: End ]--------------------------//
	
	struct timeval start, finish;
	gettimeofday(&start, NULL);

	for (int i = 0; i < count; i++)
		test();

	gettimeofday(&finish, NULL);

	printMemory(ass, 32);
	timeCalc(start, finish);

	//----------------------[ OpenSSL: Begin ]--------------------------//
	/* Removes all digests and ciphers */
	EVP_cleanup();

	/* if you omit the next, a small leak may be left when you make use of the BIO (low level API) for e.g. base64 transformations */
	CRYPTO_cleanup_all_ex_data();

	/* Remove error strings */
	ERR_free_strings();
	//----------------------[ OpenSSL: End ]--------------------------//

	return 0;
}

