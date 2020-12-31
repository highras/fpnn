#ifndef FPMessage_h_
#define FPMessage_h_

#if (__GNUC__ >= 8)
//-- For msgpack & RapidJSON: -Wall will triggered the -Wclass-memaccess with G++ 8 and later.
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

#include <memory>
#include <atomic>
#include <msgpack.hpp>
#include <endian.h>
#include "FpnnError.h"
#include "FPLog.h"
#include "msec.h"
#include "StringUtil.h"

namespace fpnn{

#define FPNN_PROTO_CURRENT_VERSION 1
#define FPNN_PROTO_SUPPORTED_VERSION 1

	class FPMessage;
	class FPQuest;
	class FPAnswer;
	typedef std::shared_ptr<FPMessage> FPMessagePtr;
	typedef std::shared_ptr<FPQuest> FPQuestPtr;
	typedef std::shared_ptr<FPAnswer> FPAnswerPtr;

	typedef msgpack::object OBJECT;
	typedef std::map<std::string, std::string> StringMap;

#define FPNN_MSGPACK_SBUFFER_INIT_SIZE 1024

	class FPMessage {
		public:
			enum FP_FLAG{
				FP_FLAG_MSGPACK		= 0x80,
				FP_FLAG_JSON		= 0x40,
				FP_FLAG_ZIP			= 0x20,
				FP_FLAG_ENCRYPT		= 0x10,
			};

			enum FP_Pack_Type {
				FP_PACK_MSGPACK		= 0,
				FP_PACK_JSON		= 1,
			};

			enum FPMessageType {
				FP_MT_ONEWAY		= 0, 
				FP_MT_TWOWAY		= 1,
				FP_MT_ANSWER		= 2,
			};

			union fpnn_magic {
				char bytes[4];
				uint32_t u32; 
			};

			struct Header {
				fpnn_magic magic;
				uint8_t version;
				uint8_t flag;
				uint8_t mtype;
				uint8_t ss;//quest method size or answer status
				uint32_t psize;
			};	


		public:
			uint8_t version() const				{ return _hdr.version; }
			uint8_t flag() const				{ return _hdr.flag; }
			uint8_t msgType() const				{ return _hdr.mtype; }
			uint8_t ss() const					{ return _hdr.ss; }
			//should not called, because if it's HTTP proto, we will not reset this value
			uint32_t payloadSize() const        { return le32toh(_hdr.psize); }

			bool isHTTP() const					{ return _hdr.magic.u32 == _POST_magic.u32 || _hdr.magic.u32 == _GET_magic.u32; } 
			bool isTCP() const					{ return _hdr.magic.u32 == _TCP_magic.u32; } 
			bool isMsgPack() const				{ return (_hdr.flag & FP_FLAG_MSGPACK) == FP_FLAG_MSGPACK; }
			bool isJson() const					{ return (_hdr.flag & FP_FLAG_JSON) == FP_FLAG_JSON; }
			bool isZip() const					{ return (_hdr.flag & FP_FLAG_ZIP) == FP_FLAG_ZIP; }
			bool isEncrypt() const				{ return (_hdr.flag & FP_FLAG_ENCRYPT) == FP_FLAG_ENCRYPT; }
			bool isOneWay() const				{ return _hdr.mtype == FP_MT_ONEWAY; }
			bool isTwoWay() const				{ return _hdr.mtype == FP_MT_TWOWAY; }
			bool isQuest() const                { return isTwoWay() || isOneWay(); }
			bool isAnswer() const               { return _hdr.mtype == FP_MT_ANSWER; }
			bool isSupportPack() const			{ return isMsgPack() != isJson(); }
			bool isSupportProto() const			{ return isTCP() || isHTTP(); }

			uint8_t methedSize() const			{ if(isQuest()) return ss(); throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_METHOD_TYPE, "Not Quest method");}
			uint8_t answerStatus() const		{ if(isAnswer()) return ss(); throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_METHOD_TYPE, "Not Answer method"); }

			uint32_t seqNum() const				{ return le32toh(_seqNum); }
			uint32_t seqNumLE() const			{ return _seqNum; }

			void setVersion(uint8_t version)	{ _hdr.version = version; }
			void setFlag(uint8_t flag)			{ _hdr.flag |= flag; }
			void unsetFlag(uint8_t flag)		{ _hdr.flag &= ~flag; }
			void setMType(uint8_t mtype)		{ _hdr.mtype = mtype; }
			void setSS(uint8_t ss)				{ _hdr.ss = ss; }
			void setPayloadSize(uint32_t size)	{ _hdr.psize = htole32(size); }
			void setSeqNum(uint32_t seqNum)		{ _seqNum = htole32(seqNum); }

			void setPayload(const std::string& payload)			{ _payload = payload; }
			void setPayload(const char* payload, size_t len)	{ _payload = std::string(payload,len); }

			const std::string& payload() const					{ return _payload; }
			//get payload json string
			std::string json();
			std::string Hex();
			void printHttpInfo();

			int64_t ctime() const								{ return _ctime; }
			void    setCTime(int64_t ctime)						{ _ctime = ctime; }
		public:
			static std::string Hex(const std::string& str);
			static uint32_t HeaderRemain()					{ return sizeof(Header) - sizeof(fpnn_magic);}
			static uint32_t BodyLen(const char* header);
			static bool isQuest(const char* header);
			static bool isTCP(const char* header){
				Header *hdr = (Header*)header;
				return hdr->magic.u32 == _TCP_magic.u32;
			}
			static bool isHTTP(const char* header){
				Header *hdr = (Header*)header;
				return hdr->magic.u32 == _POST_magic.u32 || hdr->magic.u32 == _GET_magic.u32;
			}
			static bool isMsgPack(const char* header){ 
				Header *hdr = (Header*)header;
				return (hdr->flag & FP_FLAG_MSGPACK) == FP_FLAG_MSGPACK; 
			}
			static bool isJson(const char* header){ 
				Header *hdr = (Header*)header;
				return (hdr->flag & FP_FLAG_JSON) == FP_FLAG_JSON; 
			}
			static bool isZip(const char* header){ 
				Header *hdr = (Header*)header;
				return (hdr->flag & FP_FLAG_ZIP) == FP_FLAG_ZIP; 
			}
			static bool isEncrypt(const char* header){ 
				Header *hdr = (Header*)header;
				return (hdr->flag & FP_FLAG_ENCRYPT) == FP_FLAG_ENCRYPT; 
			}
			static uint8_t version(const char* header){ 
				Header *hdr = (Header*)header;
				return hdr->version; 
			}
			static bool isSupportedVersion(const char* header){ 
				Header *hdr = (Header*)header;
				return hdr->version >= FPNN_PROTO_SUPPORTED_VERSION; 
			}
			static uint8_t flag(const char* header){ 
				Header *hdr = (Header*)header;
				return hdr->flag; 
			}
			static uint8_t msgType(const char* header){ 
				Header *hdr = (Header*)header;
				return hdr->mtype; 
			}
			static uint8_t ss(const char* header){ 
				Header *hdr = (Header*)header;
				return hdr->ss; 
			}
			static uint32_t payloadSize(const char* header) { 
				Header *hdr = (Header*)header;
				return le32toh(hdr->psize); 
			}
			static uint8_t currentVersion(){ 
				return FPNN_PROTO_CURRENT_VERSION; 
			}
			static uint8_t supportedVersion(){ 
				return FPNN_PROTO_SUPPORTED_VERSION; 
			}

		public:
			const std::string& http_uri(const std::string& u)		{ return http_infos(std::string("u_")+u); }
			const std::string& http_cookie(const std::string& c)	{ return http_infos(std::string("c_")+c);}
			const std::string& http_header(const std::string& h)	{ return http_infos(std::string("h_")+h);}
			const std::string& getWebSocket(){
				if(_hdr.magic.u32 != _GET_magic.u32)
					return _emptyString;
				if(strcasecmp(http_header("Upgrade").c_str(), "websocket") != 0)
					return _emptyString;
				if(http_header("Sec-WebSocket-Version") != "13")
					return _emptyString;
				std::string connection = http_header("Connection");
				std::set<std::string> elems;
				elems = StringUtil::split(connection, ",; ", elems);
				if(elems.find("Upgrade") == elems.end())
					return _emptyString;
				return http_header("Sec-WebSocket-Key");
			}
            StringMap http_uri_all() { return http_infos_all("u_"); }
            StringMap http_cookie_all() { return http_infos_all("c_"); }
            StringMap http_header_all() { return http_infos_all("h_"); }

		private:
			const std::string& http_infos(const std::string& k){
				if(!_httpInfos) 
					throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_UNKNOWN_ERROR, "_httpInfos is NULL");
				StringMap::iterator it = _httpInfos->find(k);
				if(it == _httpInfos->end()) return _emptyString;
				return it->second;
			}

            StringMap http_infos_all(const std::string& prefix){
                StringMap infos;
                for (auto& kv: *_httpInfos) {
                    int32_t prefixLen = prefix.length();
                    if (kv.first.substr(0, prefixLen) == prefix)
                        infos[kv.first.substr(prefixLen)] = kv.second;
                }
                return infos;
            }

		protected:
			static uint32_t nextSeqNum(){
				static std::atomic<uint32_t> nextSeq(1);
				return nextSeq++;
			}

		public:
			static fpnn_magic _TCP_magic;
			static fpnn_magic _POST_magic;
			static fpnn_magic _GET_magic;
			static Header _default_header;
			static const char* _CRLF;
			static const int16_t _CRLF_LEN;
			static std::string _emptyString;
			static const int16_t _MagicFieldLength;
			static const int16_t _HeaderLength;

			Header _hdr;
		protected:
			FPMessage():_ctime(0), _seqNum(0), _httpInfos(NULL) {}
			virtual ~FPMessage() { if(_httpInfos) delete _httpInfos; }

		protected:
			int64_t _ctime;
			uint32_t _seqNum;
			std::string _payload;
			//for HTTP, cookie, header, uri
			//key will be add c_, h_, u_
			StringMap* _httpInfos;

	};

	class FPQuest: public FPMessage{
		public:
			FPQuest(const std::string& method, bool oneway = false, FP_Pack_Type ptype = FP_PACK_MSGPACK) { 
				//will reset _ctime in take()
				if(method.empty())
					throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_INVALID_PACKAGE, "Empty method");
				_create(method, oneway, ptype); 
			}
			FPQuest(const char* method, bool oneway = false, FP_Pack_Type ptype = FP_PACK_MSGPACK) { 
				//will reset _ctime in take()
				if(method == nullptr || method[0] == '\0')
					throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_INVALID_PACKAGE, "Empty method");
				_create(std::string(method), oneway, ptype); 
			}

			//create from raw data
			FPQuest(const Header& hdr, uint32_t seq, const std::string& method, const std::string& payload){
				_ctime = slack_real_msec();
				if(method.empty())
					throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_INVALID_PACKAGE, "Empty method");
				_create(hdr, seq, method, payload);
			}
			FPQuest(const char* data, size_t len){
				_ctime = slack_real_msec();
				_create(data, len);
			}
			//create HTTP, only support json
			FPQuest(const std::string& method, const std::string& payload, StringMap& infos, bool post){
				_ctime = slack_real_msec();
				if(method.empty())
					throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_INVALID_PACKAGE, "Empty method");
				_create(method, payload, infos, post);
			}

			virtual ~FPQuest() {}
		public:
			void setMethod(const std::string& method){ 
				if(method.empty())
					throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_INVALID_PACKAGE, "Empty method");
				_method = method; 
			}
			void setMethod(const char* method){ 
				if(method == nullptr || method[0] == '\0')
					throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_INVALID_PACKAGE, "Empty method");
				_method = method; 
			}

			const std::string& method() const 			{ return _method; }

			std::string* raw();
			std::string info();

		private:
			void _create(const std::string& method, bool oneway = false, FP_Pack_Type ptype = FP_PACK_MSGPACK);
			void _create(const Header& hdr, uint32_t seq, const std::string& method, const std::string& payload);
			void _create(const char* data, size_t len);
			void _create(const std::string& method, const std::string& payload, StringMap& infos, bool post);
		private:
			std::string _method;
	};


	class FPAnswer: public FPMessage{
		public:
			enum {
				FP_ST_OK				= 0,  
				FP_ST_ERROR				= 1,  
				FP_ST_HTTP_OK			= 200,  
				FP_ST_HTTP_ERROR		= 500,  
			};
		public:
			//called by FPWriter
			FPAnswer(const FPQuestPtr quest)
				:_status(FP_ST_OK), _quest(quest) {
					//will reset _ctime in take()
					_create();
				}
			FPAnswer(uint16_t status, const FPQuestPtr quest)
				:_status(status), _quest(quest) {
					//will reset _ctime in take()
					_create();
				}

			//create from raw data
			FPAnswer(const Header& hdr, uint32_t seq, const std::string& payload)
				: _quest(NULL){ 
					_ctime = slack_real_msec();
					_create(hdr, seq, payload);
				}
			FPAnswer(const char* data, size_t len)
				: _quest(NULL){
					_ctime = slack_real_msec();
					_create(data, len);
				}
			FPAnswer(const std::string& data)
				: _quest(NULL){ 
					_ctime = slack_real_msec();
					_create(data);
				}
			virtual ~FPAnswer() {}
		public:
			void setStatus(uint16_t status)	{ _status = status; }
			uint16_t status() const			{ return _status; }

			std::string* raw();

			int64_t timeCost();

			std::string info();
			static std::string genWebsocketKey(const std::string& access);
		private:
			std::string* rawTCP();
			std::string* rawHTTP();
		private:
			void _create();
			void _create(const Header& hdr, uint32_t seq, const std::string& payload);
			void _create(const char* data, size_t len);
			void _create(const std::string& data){
				_create(data.data(), data.size());
			}
		private:
			uint16_t _status;
			FPQuestPtr _quest;
	};

}

#endif
