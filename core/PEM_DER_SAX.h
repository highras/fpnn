#ifndef FPNN_PEM_DER_SAX_H
#define FPNN_PEM_DER_SAX_H

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

#include <string>
#include <vector>

namespace fpnn
{
	/*===============================================================================
	  X.690 Reader Interface
	=============================================================================== */
	class X690ReaderInterface
	{
	public:
		enum SAXSignType {
			SAXContinue = 0,
			SAXStop = 1,
			SAXSkip = 2 		//-- Only using in enterContainer() event.
		};
	public:
		/*
			length: If less ZERO, mean length is unknown, it will be sured in exitTLV() event.
		*/
		//virtual enum SAXSignType enterTLV(int tagClass, int tag, bool constructed, long length, int depth, bool willSkipped);
		//virtual enum SAXSignType exitTLV(int tagClass, int tag, bool constructed, long length, int depth, bool skipped);


		/*
			dataLength: If less ZERO, mean data length is unknown, it will be sured in exitContainer() event.
		*/
		virtual enum SAXSignType enterContainer(int tagClass, int tag, bool constructed, long dataLength, int depth)
		{
			return SAXContinue;
		}
		virtual enum SAXSignType exitContainer(int tagClass, int tag, bool constructed, long dataLength, int depth)
		{
			return SAXContinue;
		}
		
		virtual enum SAXSignType skip(int tagClass, int tag, bool constructed, long dataLength, int depth)
		{
			return SAXContinue;
		}
		virtual enum SAXSignType objectIdentifier(int depth, const std::vector<uint64_t>& value)
		{
			return SAXContinue;
		}
		virtual enum SAXSignType BITString(int depth, int paddedBits, const std::string& value)
		{
			return SAXContinue;
		}
		virtual void documentError(const char* reason) {}
	};

	/*===============================================================================
	  X.690 / DER format SAX
	=============================================================================== */
	class X690SAX
	{
	private:
		struct ASN1Tag		//-- Only support Universal-tag
		{
			int tagClass;
			bool constructed;
			int tag;
		};
		struct ASN1Len
		{
			uint64_t length;
			int remainLenBlock;
			bool unknownLen;
		};

		struct BITStringData
		{
			std::string data;
			int padCount;
		};

	private:
		X690ReaderInterface* _reader;

	private:
		bool readTag(const char* buf, int len, int &lenOffset, ASN1Tag& asn1Tag);
		bool readLength(const char* buf, int len, int &valueOffset, ASN1Len& asn1Len);
		bool skipValue(const char* buf, int len, ASN1Tag &asn1Tag, ASN1Len &asn1Len, int &offset, int depth);
		bool praseObjectIdentifier(const char* buf, int len, ASN1Tag &asn1Tag, ASN1Len &asn1Len, int &offset, std::vector<uint64_t> &idCodes);
		bool parseBITString(const char* buf, int len, ASN1Tag &asn1Tag, ASN1Len &asn1Len, int &offset, int depth, BITStringData& bsData);
		bool parseValue(const char* buf, int len, ASN1Tag &asn1Tag, ASN1Len &asn1Len, int &offset, int depth, bool skip);
		bool parseBuffer(const char* buf, int len, int depth, bool skip, int &consumedLen);
		bool parseData(const std::string& content);

	public:
		X690SAX(): _reader(NULL) {}
		bool parse(const std::string& content, X690ReaderInterface* reader);
	};

	/*===============================================================================
	  Pem format SAX
	=============================================================================== */
	class PemSAX
	{
		std::string _header;
		std::string _footer;

	public:
		PemSAX(const std::string& header, const std::string& footer):
			_header(header), _footer(footer) {}

		PemSAX():
			PemSAX("-----BEGIN PUBLIC KEY-----", "-----END PUBLIC KEY-----") {}

		bool parse(const std::string& content, X690ReaderInterface* reader);
	};

	/*===============================================================================
	  Ecc Publick Reader
	=============================================================================== */
	class EccKeyReader: public X690ReaderInterface
	{
		enum ReaderStatusType {
			RST_wantEccId,
			RST_wantCurveName,
			RST_wantPublicKey,
			RST_completed
		};

		std::string _curve;
		std::string _publicKey;

		int _keylen;
		int _eccIdDepth;
		int _internalEventCount;
		enum ReaderStatusType _status;

	public:
		virtual enum SAXSignType enterContainer(int tagClass, int tag, bool constructed, long dataLength, int depth);
		virtual enum SAXSignType exitContainer(int tagClass, int tag, bool constructed, long dataLength, int depth);
		
		virtual enum SAXSignType skip(int tagClass, int tag, bool constructed, long dataLength, int depth);
		virtual enum SAXSignType objectIdentifier(int depth, const std::vector<uint64_t>& value);
		virtual enum SAXSignType BITString(int depth, int paddedBits, const std::string& value);
		virtual void documentError(const char* reason);

	public:
		std::string rawPublicKey() { return _publicKey; }
		std::string curveName() { return _curve; }

		EccKeyReader(): _status(RST_wantEccId) {}
	};
}

#endif
