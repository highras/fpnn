#ifndef FPReader_h_
#define FPReader_h_
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
					LOG_ERROR("EXCEPTION:%s, Cannot find object of key:%s. raw data:%s.", ex.what(), k, json().c_str());
				}
				catch(...){
					LOG_ERROR("Cannot find object of key:%s. raw data:%s.", k, json().c_str());
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

			//the following function will not throw exception
			template<typename TYPE>
				TYPE get(const char *k, TYPE dft){
					try{
						dft = _find(k).as<decltype(dft)>();
					}
					catch(const std::exception& ex){
						//LOG_INFO("EXCEPTION:%s, Cannot find value of key:%s. type:%s, raw data:%s.", ex.what(), k, typeid(dft).name(), json().c_str());
					}
					catch(...){
						//LOG_WARN("Cannot find value of key:%s. type:%s, raw data:%s.", k, typeid(dft).name(), json().c_str());
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
						throw FPNN_ERROR_CODE_FMT(FpnnHTTPError, FPNN_EC_PROTO_TYPE_CONVERT, "Can not convert key:%s to Type:%s", k, typeid(dft).name());
					}   
					catch(...){
						throw FPNN_ERROR_CODE_FMT(FpnnHTTPError, FPNN_EC_PROTO_UNKNOWN_ERROR, "Unknow error, Want value of key:%s", k);
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

		public:
			FPReader(const std::string& payload){
				unpack(payload.data(), payload.size());
			}
			FPReader(const char* buf, size_t len){
				unpack(buf, len);
			}
			FPReader(const msgpack::object& obj):_object(obj){
				if(_object.type != msgpack::type::MAP) 
					throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_MAP_VALUE, "NOT a MAP object:%d", _object.type);
			}
			virtual ~FPReader() {}
		public:
			std::string raw(){
				throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_NOT_SUPPORTED, "should not call raw");
			}   
			std::string json(){
				return JSONConvert::Msgpack2Json(_object);
			}
		private:
			void unpack(const char* buf, size_t len){
				_oh = msgpack::unpack(buf, len);
				_object = _oh.get();
				if(_object.type != msgpack::type::MAP) 
					throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_MAP_VALUE, "NOT a MAP object:%d", _object.type);
			}

			const msgpack::object& _find(const char* key);

			const msgpack::object& _find(const std::string& key){
				return _find(key.c_str());
			}
		private:
			msgpack::object_handle _oh;
			msgpack::object _object;
			static msgpack::object _nilObj;

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
