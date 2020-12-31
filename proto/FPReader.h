#ifndef FPReader_h_
#define FPReader_h_

#if (__GNUC__ >= 8)
//-- For msgpack & RapidJSON: -Wall will triggered the -Wclass-memaccess with G++ 8 and later.
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

#include <sstream>
#include <typeinfo>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <msgpack.hpp>
#include "FPMessage.h"
#include "FpnnError.h"
#include "JSONConvert.h"
#include "FileSystemUtil.h"
#include "FPLog.h"

namespace fpnn{

	class FPReader;
	class FPQReader;
	class FPAReader;
	typedef std::shared_ptr<FPReader> FPReaderPtr;
	typedef std::shared_ptr<FPQReader> FPQReaderPtr;
	typedef std::shared_ptr<FPAReader> FPAReaderPtr;

	class FPReader{
		public:
			const msgpack::object& getObject(const char *k){
				try{
					return _find(k);
				}
				catch(const std::exception& ex){
					LOG_ERROR("EXCEPTION: %s, Cannot find object of key: %s, data: %s", ex.what(), k, json().c_str());
				}
				catch(...){
					LOG_ERROR("Cannot find object of key: %s, data: %s", k, json().c_str());
				}
				return _nilObj;
			}

			const msgpack::object& getObject(const std::string& k){
				return getObject(k.c_str());
			}

			const msgpack::object& wantObject(const char *k){
				return _find(k);
			}

			const msgpack::object& wantObject(const std::string& k){
				return _find(k.c_str());
			}
			bool existKey(const char *k){
				try{
					_find(k);
					return true;
				}
				catch(...){}
				return false;
			}
			bool existKey(const std::string& k){
				return existKey(k.c_str());
			}

			//the following function will not throw exception
			template<typename TYPE>
				TYPE get(const char *k, TYPE dft){
					try{
						dft = _find(k).as<decltype(dft)>();
					}
					catch(const std::exception& ex){
						//LOG_INFO("EXCEPTION:%s, Cannot find value of key:%s type:%s, raw data:%s", ex.what(), k, typeid(dft).name(), json().c_str());
					}
					catch(...){
						//LOG_WARN("Cannot find value of key:%s type:%s, raw data:%s", k, typeid(dft).name(), json().c_str());
					}
					return dft;
				}

			template<typename TYPE>
				TYPE get(const std::string& k, TYPE dft){
					return get(k.c_str(), dft);
				}

			intmax_t getInt(const char* k, intmax_t dft = _intDef)				{ return get(k, dft); }
			intmax_t getInt(const std::string& k, intmax_t dft = _intDef)		{ return get(k.c_str(), dft); }
			uintmax_t getUInt(const char* k, uintmax_t dft = _uintDef)			{ return get(k, dft); }
			uintmax_t getUInt(const std::string& k, uintmax_t dft = _uintDef)	{ return get(k.c_str(), dft); }
			double getDouble(const char* k, double dft = _doubleDef)			{ return get(k, dft); }
			double getDouble(const std::string& k, double dft = _doubleDef)		{ return get(k.c_str(), dft); }
			bool getBool(const char* k, bool dft = _boolDef)					{ return get(k, dft); }
			bool getBool(const std::string& k, bool dft = _boolDef)				{ return get(k.c_str(), dft); }
			std::string getString(const char* k, std::string dft = _stringDef)	{ return get(k, dft); }
			std::string getString(const std::string& k, std::string dft = _stringDef){ return get(k.c_str(), dft); }
			bool getFile(const char* k, FileSystemUtil::FileAttrs& attrs);
			bool getFile(const std::string& k, FileSystemUtil::FileAttrs& attrs);

			// The following function will throw exceptions
			template<typename TYPE>
				TYPE want(const char *k, TYPE dft){
					try{
						dft = _find(k).as<decltype(dft)>();
						return dft;
					}
					catch (const FpnnError& ex){
						throw ex;
					}
					catch(const std::exception& ex){
						throw FPNN_ERROR_CODE_FMT(FpnnHTTPError, FPNN_EC_PROTO_TYPE_CONVERT, "Can not convert key: %s to Type: %s, data: %s", k, typeid(dft).name(), json().c_str());
					}   
					catch(...){
						throw FPNN_ERROR_CODE_FMT(FpnnHTTPError, FPNN_EC_PROTO_UNKNOWN_ERROR, "Unknow error, Want value of key: %s, data: %s", k, json().c_str());
					}   
				}

			template<typename TYPE>
				TYPE want(const std::string& k, TYPE dft){
					return want(k.c_str(), dft);
				}
			intmax_t wantInt(const char* k, intmax_t dft = _intDef)				{ return want(k, dft); }
			intmax_t wantInt(const std::string& k, intmax_t dft = _intDef)		{ return want(k.c_str(), dft); }
			uintmax_t wantUInt(const char* k, uintmax_t dft = _uintDef)			{ return want(k, dft); }
			uintmax_t wantUInt(const std::string& k, uintmax_t dft = _uintDef)	{ return want(k.c_str(), dft); }
			double wantDouble(const char* k, double dft = _doubleDef)			{ return want(k, dft); }
			double wantDouble(const std::string& k, double dft = _doubleDef)	{ return want(k.c_str(), dft); }
			bool wantBool(const char* k, bool dft = _boolDef)					{ return want(k, dft); }
			bool wantBool(const std::string& k, bool dft = _boolDef)			{ return want(k.c_str(), dft); }
			std::string wantString(const char* k, std::string dft = _stringDef)	{ return want(k, dft); }
			std::string wantString(const std::string& k, std::string dft = _stringDef){ return want(k.c_str(), dft); }
			bool wantFile(const char* k, FileSystemUtil::FileAttrs& attrs);
			bool wantFile(const std::string& k, FileSystemUtil::FileAttrs& attrs);

			template<typename TYPE>
				TYPE convert(TYPE& dst){
					_object.convert(dst);
					return dst;
				} 

			
			msgpack::type::object_type getObjectType(const char* k){
				return getObject(k).type;
			}
			msgpack::type::object_type getObjectType(const std::string& k){
				return getObject(k).type;
			}
			//get the type of key, if key not exist, return false
			bool isInt(const char* k){ 
				msgpack::type::object_type otype = getObjectType(k);
				if(otype == msgpack::type::POSITIVE_INTEGER || otype == msgpack::type::NEGATIVE_INTEGER)
					return true;
				return false;
			}
			bool isInt(const std::string& k)		{ return isInt(k.c_str()); }
			bool isBool(const char* k){ 
				msgpack::type::object_type otype = getObjectType(k);
				if(otype == msgpack::type::BOOLEAN)
					return true;
				return false;
			}
			bool isBool(const std::string& k)		{ return isBool(k.c_str()); }
			bool isDouble(const char* k){ 
				msgpack::type::object_type otype = getObjectType(k);
				if(otype == msgpack::type::FLOAT32 || otype == msgpack::type::FLOAT64 
						|| otype == msgpack::type::FLOAT
#if defined(MSGPACK_USE_LEGACY_NAME_AS_FLOAT)
						|| otype == msgpack::type::DOUBLE
#endif
						)
					return true;
				return false;
			}
			bool isDouble(const std::string& k)		{ return isDouble(k.c_str()); }
			bool isString(const char* k){ 
				msgpack::type::object_type otype = getObjectType(k);
				if(otype == msgpack::type::STR)
					return true;
				return false;
			}
			bool isString(const std::string& k)		{ return isString(k.c_str()); }
			bool isBinary(const char* k){ 
				msgpack::type::object_type otype = getObjectType(k);
				if(otype == msgpack::type::BIN)
					return true;
				return false;
			}
			bool isBinary(const std::string& k)		{ return isBinary(k.c_str()); }
			bool isArray(const char* k){ 
				msgpack::type::object_type otype = getObjectType(k);
				if(otype == msgpack::type::ARRAY)
					return true;
				return false;
			}
			bool isArray(const std::string& k)		{ return isArray(k.c_str()); }
			bool isMap(const char* k){ 
				msgpack::type::object_type otype = getObjectType(k);
				if(otype == msgpack::type::MAP)
					return true;
				return false;
			}
			bool isMap(const std::string& k)		{ return isMap(k.c_str()); }
			bool isExt(const char* k){ 
				msgpack::type::object_type otype = getObjectType(k);
				if(otype == msgpack::type::EXT)
					return true;
				return false;
			}
			bool isExt(const std::string& k)		{ return isExt(k.c_str()); }
			//TODO
			bool isNil(const char* k){ 
				msgpack::type::object_type otype = getObjectType(k);
				if(otype == msgpack::type::NIL)
					return true;
				return false;
			}
			bool isNil(const std::string& k)		{ return isNil(k.c_str()); }

		public:
			FPReader(const std::string& payload){
				unpack(payload.data(), payload.size());
			}
			FPReader(const char* buf, size_t len){
				unpack(buf, len);
			}
			FPReader(const msgpack::object& obj):_object(obj){
				if(_object.type != msgpack::type::MAP) 
					throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_MAP_VALUE, "NOT a MAP object: %s", json().c_str());
			}
			virtual ~FPReader() {}
		public:
			std::string raw(){
				throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_NOT_SUPPORTED, "should not call raw");
			}   
			std::string json(){
				try{
					return JSONConvert::Msgpack2Json(_object);
				}   
				catch(const std::exception& ex){
					LOG_ERROR("EXCEPTION:%s", ex.what());
				}   
				catch(...){
					LOG_ERROR("Unknow Exception");
				}   
				return "";
			}
		private:
			void unpack(const char* buf, size_t len){
				try{
					_oh = msgpack::unpack(buf, len);
					_object = _oh.get();
				}   
				catch(const std::exception& ex){
					std::string hex = FPMessage::Hex(std::string(buf, len));
					LOG_ERROR("unpack, exception: %s, len: %d, buf: %s", ex.what(), len, hex.c_str());
					throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_INVALID_PACKAGE, "Invalid Package, len: %d, buf: %s", len, hex.c_str()); 
				}   
				catch(...){
					std::string hex = FPMessage::Hex(std::string(buf, len));
					LOG_ERROR("unpack, exception, len: %d, buf: %s", len, hex.c_str());
					throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_INVALID_PACKAGE, "Invalid Package, len: %d, buf: %s", len, hex.c_str());
				}   

				if(_object.type != msgpack::type::MAP) 
					throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_MAP_VALUE, "NOT a MAP object: %s", json().c_str());
			}

			const msgpack::object& _find(const char* key);

			const msgpack::object& _find(const std::string& key){
				return _find(key.c_str());
			}
		private:
			msgpack::object_handle _oh;
			msgpack::object _object;
			static msgpack::object _nilObj;
			//_nilObj.type == msgpack::type::NIL

			static intmax_t _intDef;
			static uintmax_t _uintDef;
			static double	_doubleDef;
			static std::string _stringDef;
			static bool	_boolDef;
	};


	class FPQReader : public FPReader{
		public:
			FPQReader(const FPQuestPtr& quest)
				: FPReader(quest->payload()), _quest(quest){
				}   

			bool isHTTP()						{ return _quest->isHTTP(); }
			bool isTCP()						{ return _quest->isTCP(); }
			bool isOneWay()						{ return _quest->isOneWay(); }
			bool isTwoWay()						{ return _quest->isTwoWay(); }
			bool isQuest()						{ return isOneWay() || isTwoWay(); }
			uint32_t seqNum() const				{ return _quest->seqNum(); }
			const std::string& method() const   { return _quest->method(); }

		private:
			FPQuestPtr _quest;
	};

	class FPAReader : public FPReader{
		public:
			FPAReader(const FPAnswerPtr& answer)
				: FPReader(answer->payload()), _answer(answer){
				}   

			uint32_t seqNum() const				{ return _answer->seqNum(); }
			uint16_t status() const				{ return _answer->status(); }
		private:
			FPAnswerPtr _answer;
	};

}

#endif
