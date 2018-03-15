#include "JSONConvert.h"
#include "FpnnError.h"

using namespace fpnn;

void parseJson(const JSONConvert::FPValue& doc, msgpack::packer<msgpack::sbuffer>& pk){
	switch(doc.GetType()){
		case rapidjson::kStringType:
			pk.pack_str(doc.GetStringLength());
			pk.pack_str_body(doc.GetString(), doc.GetStringLength());
			break;
		case rapidjson::kNumberType:
			if (doc.IsInt())
				pk.pack_int(doc.GetInt());
			else if (doc.IsUint())
				pk.pack_unsigned_int(doc.GetUint());
			else if (doc.IsInt64())
				pk.pack_int64(doc.GetInt64());
			else if (doc.IsUint64())
				pk.pack_uint64(doc.GetUint64());
			else if (doc.IsDouble()||doc.IsNumber())
				pk.pack_double(doc.GetDouble());
			else 
				throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_JSON_CONVERT, "Not a valid number type");
			break;
		case rapidjson::kTrueType:
			pk.pack_true(); 
			break;
		case rapidjson::kFalseType:
			pk.pack_false();
			break;
		case rapidjson::kObjectType:
			pk.pack_map(doc.MemberCount());
			for(JSONConvert::FPValue::ConstMemberIterator it=doc.MemberBegin(); it!=doc.MemberEnd(); ++it){
				pk.pack_str(it->name.GetStringLength());
				pk.pack_str_body(it->name.GetString(), it->name.GetStringLength());
				parseJson(it->value, pk);	
			}
			break;
		case rapidjson::kArrayType:
			pk.pack_array(doc.Size());
			for (JSONConvert::FPValue::ConstValueIterator it=doc.Begin(); it!=doc.End(); ++it){
				parseJson(*it, pk);
			}
			break;
		case rapidjson::kNullType:
			pk.pack_nil();
			break;
		default:
			throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_JSON_CONVERT, "unknow Type:%d", doc.GetType());
	}
}

std::string JSONConvert::Json2Msgpack(const std::string& jbuf){
	rapidjson::Document doc;
	if(doc.Parse(jbuf.data()).HasParseError()) 
		throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_JSON_CONVERT, "Not a valid json:%s", jbuf.c_str());
	if(!doc.IsObject()) 
		throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_JSON_CONVERT, "Not a json object:%s", jbuf.c_str()); 

	msgpack::sbuffer sbuf(1024);
	msgpack::packer<msgpack::sbuffer> pk(&sbuf);

	parseJson(doc, pk);	
	return std::string(sbuf.data(), sbuf.size());
}

std::string JSONConvert::Msgpack2Json(const std::string& mbuf){
	return Msgpack2Json(mbuf.data(), mbuf.size());
}

std::string JSONConvert::Msgpack2Json(const char* buf, size_t n){
	msgpack::object_handle oh = msgpack::unpack(buf, n);
	msgpack::object  obj = oh.get();
	return Msgpack2Json(obj);
}

std::string JSONConvert::Msgpack2Json(const msgpack::object& obj){
	if(obj.type == msgpack::type::STR){
		return std::string(obj.via.str.ptr, obj.via.str.size);
	}  
	std::ostringstream os; 
	os << obj;
	return os.str();
}
