#include <iostream>
#include <vector>
#include "AutoRelease.h"
#include "base64.h"
#include "StringUtil.h"
#include "PemGenerator.h"

using namespace std;
using namespace fpnn;

void PemGenerator::encodeAndPushInteger(uint64_t code, std::string& data)
{
	std::list<uint8_t> seqeue;

	if (code)
	{
		while (code > 0x80)
		{
			seqeue.push_front(code % 128);
			code /= 128;
		}

		if (code)
			seqeue.push_front(code);
	}
	else
		seqeue.push_front(code);

	int count = 0;
	int total = (int)seqeue.size();
	for (auto it = seqeue.begin(); it != seqeue.end(); it++, count++)
	{
		if (count != total - 1)
			data.append(1, 0x80 | *it);
		else
			data.append(1, *it);
	}
}

std::string PemGenerator::buildTLV_ObjectIdentifier(const char* idCode)
{
	std::vector<uint64_t> codes;
	StringUtil::split(idCode, ".", codes);

	//-- ASN.1 Rules check. Refer: ITU-T X.208
	if (codes.size() < 2 || codes[0] > 2)
		return std::string();

	if (codes[0] < 2 && codes[1] > 39)
		return std::string();

	std::string bin;
	uint64_t section = codes[0] * 40 + codes[1];
	pushOIDSectionCode(section, bin);

	for (size_t i = 2; i < codes.size(); i++)
		pushOIDSectionCode(codes[i], bin);

	uint8_t tag = 6;
	std::string data(1, tag);
	pushFixedLength(bin.size(), data);
	data.append(bin);

	return data;
}

std::string PemGenerator::buildTLV_BITString(const std::string& rawData)
{
	std::string value(1, 0);
	value.append(rawData);

	std::string data(1, 0x3);
	pushFixedLength(value.size(), data);
	data.append(value);

	return data;
}

std::string PemGenerator::buildTLV_Sequence(const std::list<std::string*>& sequence, int totalLen)
{
	std::string bin(1, 0x20 | 0x10);
	pushFixedLength(totalLen, bin);
	for (auto it = sequence.begin(); it != sequence.end(); it++)
		bin.append(*(*it));

	return bin;
}

bool PemGenerator::buildPemContent()
{
	base64_t b64;
	if (base64_init(&b64, (const char *)std_base64.alphabet) < 0)
	{
		_pemData.clear();
		cout<<"Init for base64 failed."<<endl;
		return false;
	}

	char *buf = new char[BASE64_LEN(_derData.length())];
	AutoDeleteArrayGuard<char> guard(buf);

	int len = base64_encode(&b64, buf, _derData.data(), _derData.length(), BASE64_AUTO_NEWLINE);
	if (len < 0)
	{
		cout<<"Encode base64 failed."<<endl;
		return false;
	}

	_pemData = "-----BEGIN PUBLIC KEY-----\n";
	_pemData.append(buf, len);
	_pemData.append(1, '\n');
	_pemData.append("-----END PUBLIC KEY-----\n");

	return true;
}

bool PemGenerator::build(const char* curevName, const uint8_t* publicKey)
{
	size_t keyLen;
	std::string curveObjId;

	if (strcmp(curevName, "secp256k1") == 0)
	{
		curveObjId = buildTLV_ObjectIdentifier("1.3.132.0.10");
		keyLen = 32 * 2;
	}
	else if (strcmp(curevName, "secp256r1") == 0)
	{
		curveObjId = buildTLV_ObjectIdentifier("1.2.840.10045.3.1.7");
		keyLen = 32 * 2;
	}
	else if (strcmp(curevName, "secp224r1") == 0)
	{
		curveObjId = buildTLV_ObjectIdentifier("1.3.132.0.33");
		keyLen = 28 * 2;
	}
	else if (strcmp(curevName, "secp192r1") == 0)
	{
		curveObjId = buildTLV_ObjectIdentifier("1.2.840.10045.3.1.1");
		keyLen = 24 * 2;
	}
	else
	{
		cout<<"Unsupported ECC curve."<<endl;
		return false;
	}

	std::string eccObjId = buildTLV_ObjectIdentifier("1.2.840.10045.2.1");

	std::string keyContent(1, 0x04);		//-- Refer: https://tools.ietf.org/html/rfc5480#section-2.2
	keyContent.append((char*)publicKey, keyLen);
	std::string keyData = buildTLV_BITString(keyContent);

	std::list<std::string*> eccIdsSeq{ &eccObjId, &curveObjId };
	std::string eccIdsTLVBin = buildTLV_Sequence(eccIdsSeq, eccObjId.length() + curveObjId.length());

	std::list<std::string*> derContentSeq{ &eccIdsTLVBin, &keyData };
	_derData = buildTLV_Sequence(derContentSeq, eccIdsTLVBin.length() + keyData.length());

	return buildPemContent();
}