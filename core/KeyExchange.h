#ifndef FPNN_KeyExchange_h
#define FPNN_KeyExchange_h

#include <string>
#include <stdint.h>
#include "micro-ecc/uECC.h"

/*
	Config Item:
	FPNN.server.security.ecdh.enable = false
	FPNN.server.security.ecdh.curve = 
	FPNN.server.security.ecdh.privateKey = 

	curve:
		secp256k1
		secp256r1
		secp224r1
		secp192r1
	privateKey:
		path for file to recorded pravate key binary data.
*/

namespace fpnn
{
	class ECCKeyExchange		//-- Server using.
	{
	protected:
		int _secertLen;
		uECC_Curve _curve;
		std::string _privateKey;

	public:
		ECCKeyExchange(): _secertLen(0), _curve(NULL) {}

		/*
			init() auto load config items:
				FPNN.server.security.ecdh.curve = 
				FPNN.server.security.ecdh.privateKey = 
		*/
		bool init();
		bool init(const char* proto);
		bool init(const std::string& curve, const std::string& privateKey);
		/*
			key: OUT. Key buffer length is equal to keylen.
			iv: OUT. iv buffer length is 16 bytes.
			keylen: IN. 16 or  32.
			peerPublicKey: IN.
		*/
		bool calcKey(uint8_t* key, uint8_t* iv, int keylen, const std::string& peerPublicKey);
	};

	//-- Client using.
	class ECCKeysMaker: public ECCKeyExchange
	{
		std::string _publicKey;
		std::string _peerPublicKey;

	public:
		ECCKeysMaker() {}
		void setPeerPublicKey(const std::string& peerPublicKey)
		{
			_peerPublicKey = peerPublicKey;
		}
		bool setCurve(const std::string& curve);
		std::string publicKey(bool reGen = false);

		inline bool calcKey(uint8_t* key, uint8_t* iv, int keylen)
		{
			return ECCKeyExchange::calcKey(key, iv, keylen, _peerPublicKey);
		}
	};
}

#endif
