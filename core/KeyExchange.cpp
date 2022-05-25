#include "FPLog.h"
#include "md5.h"
#include "sha256.h"
#include "Setting.h"
#include "FileSystemUtil.h"
#include "KeyExchange.h"

using namespace fpnn;

bool ECCKeyExchange::init()
{
	std::string curve = Setting::getString("FPNN.server.security.ecdh.curve");
	std::string file = Setting::getString("FPNN.server.security.ecdh.privateKey");
	
	if (curve.empty() || file.empty())
		return false;

	std::string privateKey;
	if (FileSystemUtil::readFileContent(file, privateKey) == false)
	{
		LOG_ERROR("Read private key file %s failed.", file.c_str());
		return false;
	}

	return init(curve, privateKey);
}

bool ECCKeyExchange::init(const char* proto)
{
	std::vector<std::string> priorCurveKeys{ std::string("FPNN.server.").append(proto).append(".security.ecdh.curve"), "FPNN.server.security.ecdh.curve"};
	std::vector<std::string> priorFileKeys{ std::string("FPNN.server.").append(proto).append(".security.ecdh.privateKey"), "FPNN.server.security.ecdh.privateKey"};

	std::string curve = Setting::getString(priorCurveKeys);
	std::string file = Setting::getString(priorFileKeys);
	
	if (curve.empty() || file.empty())
		return false;

	std::string privateKey;
	if (FileSystemUtil::readFileContent(file, privateKey) == false)
	{
		LOG_ERROR("Read private key file %s failed.", file.c_str());
		return false;
	}

	return init(curve, privateKey);
}

bool ECCKeyExchange::init(const std::string& curve, const std::string& privateKey)
{
	if (curve == "secp256k1")
	{
		_curve = uECC_secp256k1();
		_secertLen = 32;
	}
	else if (curve == "secp256r1")
	{
		_curve = uECC_secp256r1();
		_secertLen = 32;
	}
	else if (curve == "secp224r1")
	{
		_curve = uECC_secp224r1();
		_secertLen = 28;
	}
	else if (curve == "secp192r1")
	{
		_curve = uECC_secp192r1();
		_secertLen = 24;
	}
	else
	{
		LOG_ERROR("Unsupported ECC curve.");
		return false;
	}

	if ((int)privateKey.length() != uECC_curve_private_key_size(_curve))
	{
		LOG_ERROR("Private length missmatched.");
		return false;
	}

	_privateKey = privateKey;
	return true;
}

bool ECCKeyExchange::calcKey(uint8_t* key, uint8_t* iv, int keylen, const std::string& peerPublicKey)
{
	if (!_curve)
	{
		LOG_FATAL("ECC Private Key Config ERROR.");
		return false;
	}
	if ((int)peerPublicKey.length() != _secertLen * 2)
	{
		LOG_ERROR("Peer public key length missmatched.");
		return false;
	}

	uint8_t secret[32];
	int r = uECC_shared_secret((uint8_t*)peerPublicKey.data(), (uint8_t*)_privateKey.data(), secret, _curve);
	if (r == 0)
	{
		LOG_ERROR("Cacluate shared secret failed.");
		return false;
	}

	if (keylen == 16)
	{
		memcpy(key, secret, 16);
	}
	else if (keylen == 32)
	{
		if (_secertLen == 32)
			memcpy(key, secret, 32);
		else
			sha256_checksum(key, secret, _secertLen);
	}
	else
	{
		LOG_ERROR("ECC Key Exchange: key len error.");
		return false;
	}

	md5_checksum(iv, secret, _secertLen);
	return true;
}


bool ECCKeysMaker::setCurve(const std::string& curve)
{
	if (curve == "secp256k1")
	{
		_curve = uECC_secp256k1();
		_secertLen = 32;
	}
	else if (curve == "secp256r1")
	{
		_curve = uECC_secp256r1();
		_secertLen = 32;
	}
	else if (curve == "secp224r1")
	{
		_curve = uECC_secp224r1();
		_secertLen = 28;
	}
	else if (curve == "secp192r1")
	{
		_curve = uECC_secp192r1();
		_secertLen = 24;
	}
	else
		return false;

	_publicKey.clear();
	_privateKey.clear();

	return true;
}

std::string ECCKeysMaker::publicKey(bool reGen)
{
	if (!_curve)
	{
		LOG_FATAL("ECC Private Key Config ERROR.");
		return std::string();
	}

	if (_publicKey.empty() || reGen)
	{
		uint8_t privateKey[32];
		uint8_t publicKey[64];

		if (uECC_make_key(publicKey, privateKey, _curve))
		{
			_publicKey.assign((char*)publicKey, _secertLen * 2);
			_privateKey.assign((char*)privateKey, _secertLen);
		}
		else
		{
			LOG_ERROR("Gen public key & private key failed.");
			return std::string();
		}
	}

	return _publicKey;
}
