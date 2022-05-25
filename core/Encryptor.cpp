#ifdef __APPLE__
	#include "Endian.h"
#else
	#include <endian.h>
#endif
#include "Encryptor.h"

using namespace fpnn;

void PackageEncryptor::decrypt(uint8_t* dest, uint8_t* src, int len)
{
	uint8_t iv[16];
	memcpy(iv, _iv, 16);

	size_t pos = 0;
	rijndael_context deCtx;
	rijndael_setup_encrypt(&deCtx, _key, _keyLen);

	rijndael_cfb_encrypt(&deCtx, false, src, dest, len, iv, &pos);
}

void PackageEncryptor::encrypt(uint8_t* dest, uint8_t* src, int len)
{
	uint8_t iv[16];
	memcpy(iv, _iv, 16);

	size_t pos = 0;
	rijndael_context enCtx;
	rijndael_setup_encrypt(&enCtx, _key, _keyLen);

	rijndael_cfb_encrypt(&enCtx, true, src, dest, len, iv, &pos);
}

void PackageEncryptor::encrypt(std::string* buffer)
{
	uint8_t iv[16];
	memcpy(iv, _iv, 16);

	size_t pos = 0;
	rijndael_context enCtx;
	rijndael_setup_encrypt(&enCtx, _key, _keyLen);

	const size_t int32Len = sizeof(int32_t);
	size_t buf_len = buffer->length() + int32Len;
	char* buf = (char*)malloc(buf_len);

	*((uint32_t*)buf) = htole32((uint32_t)(buffer->length()));
	rijndael_cfb_encrypt(&enCtx, true, (uint8_t *)buffer->data(), (uint8_t *)(buf + int32Len), buffer->length(), iv, &pos);
	buffer->assign(buf, buf_len);
	free(buf);
}

void StreamEncryptor::decrypt(uint8_t* dest, uint8_t* src, int len)
{
	rijndael_cfb_encrypt(&_ctx, false, src, dest, len, _iv, &_pos);
}

void StreamEncryptor::encrypt(uint8_t* dest, uint8_t* src, int len)
{
	rijndael_cfb_encrypt(&_ctx, true, src, dest, len, _iv, &_pos);
}

void StreamEncryptor::encrypt(std::string* buffer)
{
	size_t buf_len = buffer->length();
	char* buf = (char*)malloc(buf_len);
	rijndael_cfb_encrypt(&_ctx, true, (uint8_t *)buffer->data(), (uint8_t *)buf, buffer->length(), _iv, &_pos);
	buffer->assign(buf, buf_len);
	free(buf);
}
