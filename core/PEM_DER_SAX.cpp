#include <list>
#include <string.h>
#include "AutoRelease.h"
#include "base64.h"
#include "FPLog.h"
#include "StringUtil.h"
#include "PEM_DER_SAX.h"

using namespace fpnn;

/*===============================================================================
  X.690 / DER format SAX
=============================================================================== */
bool X690SAX::readTag(const char* buf, int len, int &lenOffset, ASN1Tag& asn1Tag)
{
	lenOffset = 1;

	asn1Tag.tagClass = 0xc0 & buf[0];
	asn1Tag.tagClass = asn1Tag.tagClass >> 6;
	asn1Tag.constructed = 0x20 & buf[0];

	asn1Tag.tag = 0x1f & buf[0];
	if (asn1Tag.tag != 0x1f)
		return true;

	for (int i = 1; i < len; i++)
	{
		if ((0x80 & buf[i]) == 0)
		{
			lenOffset = i + 1;
			return true;
		}
	}

	return false;
}

bool X690SAX::readLength(const char* buf, int len, int &valueOffset, ASN1Len& asn1Len)
{
	valueOffset = 1;

	asn1Len.length = 0;
	asn1Len.remainLenBlock = 0;
	asn1Len.unknownLen = false;

	if ((uint8_t)(buf[0]) == 0x80)
	{
		asn1Len.length = 0;
		asn1Len.unknownLen = true;
		return true;
	}

	if (buf[0] & 0x80)
	{
		asn1Len.length = 0;
		asn1Len.remainLenBlock = buf[0] & 0x7f;

		valueOffset = asn1Len.remainLenBlock + 1;
		if (asn1Len.remainLenBlock > 8)
		{
			_reader->documentError("Get a invalid huge block length.");
			return false;
		}

		if (len < asn1Len.remainLenBlock)
		{
			_reader->documentError("TLV length section requires length is lager than buffer length.");
			return false;
		}

		for (int i = 1; asn1Len.remainLenBlock > 0; i++)
		{
			asn1Len.length = asn1Len.length * 256 + (uint8_t)buf[i];
			asn1Len.remainLenBlock -= 1;
		}

		return true;
	}
	else
	{
		asn1Len.length = buf[0] & 0x7f;
		return true;
	}
}

bool X690SAX::skipValue(const char* buf, int len, ASN1Tag &asn1Tag, ASN1Len &asn1Len, int &offset, int depth)
{
	if (!asn1Len.unknownLen)
	{
		offset = asn1Len.length;
		return true;
	}

	//-- unknown length
	if (asn1Tag.constructed)
	{
		offset = 0;
		while (len - offset > 0)
		{
			int consumedLen;
			if (parseBuffer(buf + offset, len - offset, depth, true, consumedLen) == false)
				return false;

			offset += consumedLen;

			if (buf[offset] == 0 && buf[offset + 1] == 0)
			{
				offset += 2;
				return true;
			}
		}

		_reader->documentError("Unknown length section without end-of-contents octets.");
		return false;
	}
	else
	{
		bool lastZero = false;
		for (int i = 0; i < len; i++)
		{
			if (buf[i] == 0)
			{
				if (lastZero)
				{
					offset = i + 1;
					return true;
				}
				else
					lastZero = true;
			}
			else
				lastZero = false;
		}

		_reader->documentError("Unknown length section without end-of-contents octets.");
		return false;
	}
}

bool X690SAX::praseObjectIdentifier(const char* buf, int len, ASN1Tag &asn1Tag, ASN1Len &asn1Len, int &offset, std::vector<uint64_t> &idCodes)
{
	offset = 0;
	uint64_t code = 0;

	while (len - offset > 0)
	{
		if (0x80 & buf[offset])
		{
			code = code * 128 + (uint8_t)(buf[offset] & 0x7f);
			offset += 1;
			continue;
		}
		else
			code = code * 128 + (uint8_t)(buf[offset]);

		if (idCodes.empty())
		{
			idCodes.push_back(code / 40);
			idCodes.push_back(code % 40);
		}
		else
			idCodes.push_back(code);

		code = 0;
		offset += 1;

		if (asn1Len.unknownLen && buf[offset] == 0 && buf[offset + 1] == 0)
		{
			offset += 2;
			return true;
		}
		else
		{
			if ((uint64_t)offset == asn1Len.length)
				return true;
		}
	}

	if (asn1Len.unknownLen)
	{
		_reader->documentError("Unknown length section without end-of-contents octets.");
		return false;
	}
	else if (len - offset >= 0)
		return true;
	else
	{
		_reader->documentError("Document may be truncated.");
		return false;
	}
}

bool X690SAX::parseBITString(const char* buf, int len, ASN1Tag &asn1Tag, ASN1Len &asn1Len, int &offset, int depth, BITStringData& bsData)
{
	std::list<uint8_t> bitChain;

	if (asn1Tag.constructed == false)
	{
		if (asn1Len.unknownLen)
		{
			_reader->documentError("Data encoding error. BIT String in primitive encoding but its total length is unknown.");
			return false;
		}

		offset = asn1Len.length;

		bsData.padCount = (uint8_t)(buf[0]);
		if (asn1Len.length > 1)
			bsData.data.assign(buf + 1, asn1Len.length - 1);

		return true;
	}
	else
	{
		_reader->documentError("Find constructed BIT String. Constructed BIT String is unsupported currently.");
		return false;
		//return skipValue(buf, len, asn1Tag, asn1Len, offset, depth);
	}

	return true;
}

bool X690SAX::parseValue(const char* buf, int len, ASN1Tag &asn1Tag, ASN1Len &asn1Len, int &offset, int depth, bool skip)
{
	//printf("%sd=%2d, l=%4d, tag: %u\n", (skip ? "(skip) " : ""), depth, (asn1Len.unknownLen ? -1 : asn1Len.length), asn1Tag.tag);
	long dataLen = asn1Len.unknownLen ? -1 : asn1Len.length;

	if (asn1Tag.tagClass != 0x00)
	{
		if (_reader->skip(asn1Tag.tagClass, asn1Tag.tag, asn1Tag.constructed, dataLen, depth) == X690ReaderInterface::SAXStop)
		{
			_reader->documentError("User stopped.");
			return false;
		}
		return skipValue(buf, len, asn1Tag, asn1Len, offset, depth);
	}

	//-- BIT STRING
	if (asn1Tag.tag == 3)
	{
		BITStringData bsData;
		if (parseBITString(buf, len, asn1Tag, asn1Len, offset, depth, bsData) == false)
			return false;
		
		if (_reader->BITString(depth, bsData.padCount, bsData.data) == X690ReaderInterface::SAXStop)
		{
			_reader->documentError("User stopped.");
			return false;
		}
		return true;
	}
	//-- OBJECT IDENTIFIER
	else if (asn1Tag.tag == 6)
	{
		std::vector<uint64_t> idCodes;
		if (praseObjectIdentifier(buf, len, asn1Tag, asn1Len, offset, idCodes) == false)
			return false;

		if (idCodes.empty())
		{
			_reader->documentError("Empty Object Idenifier.");
			return false;
		}

		if (_reader->objectIdentifier(depth, idCodes) == X690ReaderInterface::SAXStop)
		{
			_reader->documentError("User stopped.");
			return false;
		}
		return true;
	}
	//-- SEQUENCE and SEQUENCE OF
	else if (asn1Tag.tag == 16)
	{
		offset = 0;

		//-- Enter Container event
		X690ReaderInterface::SAXSignType revCode;
		revCode = _reader->enterContainer(asn1Tag.tagClass, asn1Tag.tag, asn1Tag.constructed, dataLen, depth);		
		if (revCode == X690ReaderInterface::SAXStop)
		{
			_reader->documentError("User stopped.");
			return false;
		}
		else if (revCode == X690ReaderInterface::SAXSkip)
		{
			if (skipValue(buf, len, asn1Tag, asn1Len, offset, depth) == false)
				return false;

			if (_reader->exitContainer(asn1Tag.tagClass, asn1Tag.tag, asn1Tag.constructed, offset, depth) == X690ReaderInterface::SAXStop)
			{
				_reader->documentError("User stopped.");
				return false;
			}
			return true;
		}

		//-- Parse Container members
		while (len - offset > 0)
		{
			int consumedLen;
			if (parseBuffer(buf + offset, len - offset, depth + 1, false, consumedLen) == false)
				return false;

			offset += consumedLen;

			if (asn1Len.unknownLen && buf[offset] == 0 && buf[offset + 1] == 0)
			{
				offset += 2;
				break;
			}
			else
			{
				if ((uint64_t)offset == asn1Len.length)
					break;
			}
		}

		if (asn1Len.unknownLen && len - offset <= 0)
		{
			_reader->documentError("Unknown length section without end-of-contents octets.");
			return false;
		}
		else if (len - offset < 0)
		{
			_reader->documentError("Document may be truncated.");
			return false;
		}

		//-- Exit Container event
		if (_reader->exitContainer(asn1Tag.tagClass, asn1Tag.tag, asn1Tag.constructed, offset, depth) == X690ReaderInterface::SAXStop)
		{
			_reader->documentError("User stopped.");
			return false;
		}

		return true;
	}
	else
	{
		if (_reader->skip(asn1Tag.tagClass, asn1Tag.tag, asn1Tag.constructed, dataLen, depth) == X690ReaderInterface::SAXStop)
		{
			_reader->documentError("User stopped.");
			return false;
		}
		return skipValue(buf, len, asn1Tag, asn1Len, offset, depth);
	}

	return true;
}

bool X690SAX::parseBuffer(const char* buf, int len, int depth, bool skip, int &consumedLen)
{
	char *pbuf = (char *)buf;
	consumedLen = 0;

	int offset;
	ASN1Tag asn1Tag;
	ASN1Len asn1Len;

	if (readTag(pbuf, len, offset, asn1Tag) == false)
		return false;

	len -= offset;
	pbuf += offset;
	consumedLen += offset;

	if (len <= 0)
	{
		_reader->documentError("Buffer is less than zero.");
		return false;
	}

	if (readLength(pbuf, len, offset, asn1Len) == false)
		return false;

	len -= offset;
	pbuf += offset;
	consumedLen += offset;

	if (len < 0)
	{
		_reader->documentError("Buffer is less than zero.");
		return false;
	}

	//--- value
	if (parseValue(pbuf, len, asn1Tag, asn1Len, offset, depth, skip) == false)
		return false;

	len -= offset;
	pbuf += offset;
	consumedLen += offset;

	if (len < 0)
	{
		_reader->documentError("Buffer is less than zero.");
		return false;
	}

	return true;
}

bool X690SAX::parseData(const std::string& content)
{
	char* data = (char *)content.data();
	int len = (int)content.length();

	while (len > 0)
	{
		int consumedLen;
		if (parseBuffer(data, len, 0, false, consumedLen) == false)
		{
			_reader->documentError("Parse Data failed.");
			return false;
		}

		data += consumedLen;
		len -= consumedLen;
	}

	return true;
}

bool X690SAX::parse(const std::string& content, X690ReaderInterface* reader)
{
	_reader = reader;
	bool rev = parseData(content);
	_reader = NULL;
	return rev;
}

/*===============================================================================
  Pem format SAX
=============================================================================== */
bool PemSAX::parse(const std::string& content, X690ReaderInterface* reader)
{
	if (content.compare(0, _header.length(), _header) != 0)
	{
		reader->documentError("Header dismatch.");
		return false;
	}

	int adjustLen = 0;
	if (content[content.length() - 1] == '\n')
		adjustLen = 1;

	if (content.compare(content.length() - _footer.length() - adjustLen, _footer.length(), _footer) != 0)
	{
		reader->documentError("Footer dismatch.");
		return false;
	}

	base64_t b64;
	if (base64_init(&b64, (const char *)std_base64.alphabet) < 0)
	{
		reader->documentError("Init for decode base64 failed.");
		return false;
	}

	char *buf = new char[content.length()];
	AutoDeleteArrayGuard<char> guard(buf);

	memset(buf, 0, content.length());
	int len = base64_decode(&b64, buf, content.data() + _header.length(),
		content.length() - _header.length() - _footer.length() - adjustLen, BASE64_IGNORE_SPACE);
	if (len < 0)
	{
		reader->documentError("Decode base64 failed.");
		return false;
	}

	X690SAX derSAX;
	return derSAX.parse(std::string(buf, len), reader);
}

/*===============================================================================
  Ecc Publick Reader
=============================================================================== */
enum X690ReaderInterface::SAXSignType EccKeyReader::enterContainer(int tagClass, int tag, bool constructed, long dataLength, int depth)
{
	if (_status == RST_wantCurveName || _status == RST_wantPublicKey)
	{
		LOG_ERROR("Unstandard format. Please refer RFC-5480.");
		return SAXStop;
	}

	return SAXContinue;
}

enum X690ReaderInterface::SAXSignType EccKeyReader::exitContainer(int tagClass, int tag, bool constructed, long dataLength, int depth)
{
	if (_status == RST_wantCurveName)
	{
		LOG_ERROR("Unstandard format. Please refer RFC-5480.");
		return SAXStop;
	}

	if (_status == RST_wantPublicKey)
		_internalEventCount += 1;

	return SAXContinue;
}

enum X690ReaderInterface::SAXSignType EccKeyReader::skip(int tagClass, int tag, bool constructed, long dataLength, int depth)
{
	if (_status == RST_wantCurveName || _status == RST_wantPublicKey)
	{
		LOG_ERROR("Unstandard format. Please refer RFC-5480.");
		return SAXStop;
	}

	return SAXContinue;
}

enum X690ReaderInterface::SAXSignType EccKeyReader::objectIdentifier(int depth, const std::vector<uint64_t>& value)
{
	const char *OID_eccPublicKey = "1,2,840,10045,2,1";
	const char *OID_secp192r1 = "1,2,840,10045,3,1,1";
	const char *OID_secp224r1 = "1,3,132,0,33";
	const char *OID_secp256r1 = "1,2,840,10045,3,1,7";
	const char *OID_secp256k1 = "1,3,132,0,10";

	std::string oid = StringUtil::join(value, ",");

	if (_status == RST_wantEccId && oid.compare(OID_eccPublicKey) == 0)
	{
		_status = RST_wantCurveName;
		_internalEventCount = 0;
		_eccIdDepth = depth;
	}

	else if (_status == RST_wantCurveName)
	{
		if (oid.compare(OID_secp256k1) == 0)
		{
			_curve = "secp256k1";
			_keylen = 32 * 2;
		}
		else if (oid.compare(OID_secp256r1) == 0)
		{
			_curve = "secp256r1";
			_keylen = 32 * 2;
		}
		else if (oid.compare(OID_secp224r1) == 0)
		{
			_curve = "secp224r1";
			_keylen = 28 * 2;
		}
		else if (oid.compare(OID_secp192r1) == 0)
		{
			_curve = "secp192r1";
			_keylen = 24 * 2;
		}
		else
		{
			LOG_ERROR("Unexpected or unsupported ecc curve OBject Identifier.");
			return SAXStop;
		}

		if (_eccIdDepth != depth || _internalEventCount)
		{
			LOG_ERROR("Unstandard format. Please refer RFC-5480.");
			return SAXStop;
		}

		_status = RST_wantPublicKey;
	}

	else if (_status == RST_wantPublicKey)
	{
		LOG_ERROR("Unstandard format. Please refer RFC-5480.");
		return SAXStop;
	}

	return SAXContinue;
}

enum X690ReaderInterface::SAXSignType EccKeyReader::BITString(int depth, int paddedBits, const std::string& value)
{
	if (_status == RST_wantCurveName)
	{
		LOG_ERROR("Unstandard format. Please refer RFC-5480.");
		return SAXStop;
	}

	if (_status == RST_wantPublicKey)
	{
		if (paddedBits || value.length() != (size_t)_keylen + 1)
		{
			LOG_ERROR("Public key length error.");
			return SAXStop;
		}

		if (value[0] != 0x04)
		{
			LOG_ERROR("Public key error. Requrie uncompressed public key.");
			return SAXStop;
		}

		_publicKey.assign(value.data() + 1, value.length() - 1);
		_status = RST_completed;
	}

	return SAXContinue;
}

void EccKeyReader::documentError(const char* reason)
{
	LOG_ERROR(reason);
}
