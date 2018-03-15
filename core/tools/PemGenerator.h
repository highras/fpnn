/*
Reference:
	Pem:
		https://www.ietf.org/rfc/rfc5480.txt
		https://en.wikipedia.org/wiki/X.509

	ASN.1 format:
		ITU-T X.208
		ITU-T X.680
		ITU-T X.690

	Der format:
		https://en.wikipedia.org/wiki/X.690
		http://image.sciencenet.cn/olddata/kexue.com.cn/bbs/upload/11728asn1berder.pdf
		https://www.itu.int/ITU-T/studygroups/com17/languages/X.690-0207.pdf
*/

#include <list>
#include <string>

class PemGenerator
{
	std::string _derData;
	std::string _pemData;

	void encodeAndPushInteger(uint64_t code, std::string& data);
	void pushFixedLength(uint64_t code, std::string& data)
	{
		encodeAndPushInteger(code, data);
	}
	void pushOIDSectionCode(uint64_t code, std::string& data)
	{
		encodeAndPushInteger(code, data);
	}
	std::string buildTLV_ObjectIdentifier(const char* idCode);
	std::string buildTLV_BITString(const std::string& rawData);
	std::string buildTLV_Sequence(const std::list<std::string*>& sequence, int totalLen);
	bool buildPemContent();

public:
	bool build(const char* curevName, const uint8_t* publicKey);
	inline std::string getPemContent() { return _pemData; }
	inline std::string getDerContent() { return _derData; }
};
