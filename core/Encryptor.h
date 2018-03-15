#ifndef FPNN_Encryptor_H
#define FPNN_Encryptor_H

#include <stdint.h>
#include <string.h>
#include <string>
#include "rijndael.h"

namespace fpnn
{
	class Encryptor
	{
	protected:
		uint8_t _iv[16];
		uint8_t _key[32];
		size_t _keyLen;

	public:
		Encryptor(uint8_t *key, size_t key_len, uint8_t *iv)
		{
			memcpy(_key, key, key_len);
			memcpy(_iv, iv, 16);
			_keyLen = key_len;
		}
		virtual ~Encryptor() {}

		virtual void decrypt(uint8_t* dest, uint8_t* src, int len) = 0;
		virtual void encrypt(std::string* buffer) = 0;
	};

	class PackageEncryptor: public Encryptor
	{
	public:
		PackageEncryptor(uint8_t *key, size_t key_len, uint8_t *iv):
			Encryptor(key, key_len, iv) {}
		virtual ~PackageEncryptor() {}

		virtual void decrypt(uint8_t* dest, uint8_t* src, int len);
		virtual void encrypt(std::string* buffer);
	};

	class StreamEncryptor: public Encryptor
	{
		rijndael_context _ctx;
		size_t _pos;

	public:
		StreamEncryptor(uint8_t *key, size_t key_len, uint8_t *iv):
			Encryptor(key, key_len, iv), _pos(0)
		{
			rijndael_setup_encrypt(&_ctx, (const uint8_t *)_key, key_len);
		}

		virtual ~StreamEncryptor() {}

		virtual void decrypt(uint8_t* dest, uint8_t* src, int len);
		virtual void encrypt(std::string* buffer);
	};
}

#endif
