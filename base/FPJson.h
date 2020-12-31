#ifndef FPNN_JSON_H
#define FPNN_JSON_H

/************************************************
*
*	!!! Important !!!
*   !!! 非常重要 !!!
*
*	FPJson 的设计初衷是便利性，不是性能。
*	如果对性能有很高要求，请使用 RapidJson，或者其他更快的 json 库。
*
************************************************/

#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <array>
#include <deque>
#include <list>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <memory>
#include <tuple>
#include <queue>
//#include "IChainBuffer.h"  //-- support for future.
#include "FpnnError.h"

namespace fpnn {
/*******************************************************************
  Exception
*******************************************************************/
FPNN_(FpnnLogicError, FpnnJosnNodeExistError)
FPNN_(FpnnLogicError, FpnnJosnNodeNotExistError)
FPNN_(FpnnLogicError, FpnnJosnInvalidContentError)
FPNN_(FpnnLogicError, FpnnJsonNodeTypeMissMatchError)

class Json;
typedef std::shared_ptr<Json> JsonPtr;

std::ostream& operator << (std::ostream& os, const Json& node);
std::ostream& operator << (std::ostream& os, const JsonPtr node);

/*******************************************************************
 NOTICE:

 	API Group:
 		setXXX(): change the node type and value;
 		addXXX(): if node is array or object, add as sub-node;
 		pushXXX(): if node is array or object, add as sub-node;
 		mergeXXX(): if node is array or object, merge data into current node. (only for containers)
 		getXXX(): fetch node or value, if failed, return nullptr or default value, no throw exceptions;
		wantXXX(): fetch node or value, if failed, throw exceptions which is sub-type of FpnnLogicError.

 		more APIs: please see FPJsonAssistant.h.inc.

 	Empty string as key in Object:
 		Because ECMA-404 (The JSON Data Interchange Standard) is not disable the empty as key in object,
 		so path such as "..//../." means it has NINE step object and the keys are empty string.

 	Number:
 		support NaN & Infinity (inf) (ignore the cases).

	*** Only Support utf-8 ***
 	If you want this class support UCS-2, UCS-4, utf-16, utf-32,
 	and BOM, please information me.

 	All throwed exceptions are the sub-type of FpnnLogicError.
*******************************************************************/
class Json
{
public:
	enum ElementType
	{
		JSON_Object,
		JSON_Array,
		JSON_String,
		JSON_Integer,
		JSON_UInteger,
		JSON_Real,
		JSON_Boolean,
		JSON_Null,
		JSON_Uninit
	};

private:
	enum ElementType _type;

	/* ----------------------------------------------
		void *_data maybe:
			NULL: when type is null, or boolean
			not NULL: when type is boolean
			intmax_t *
			uintmax_t *
			double *
			string *
			std::list<JsonPtr> *
			std::map<std::string, JsonPtr> *
	---------------------------------------------- */
	void *_data;

	void clean();

	std::ostream& output(std::ostream& os) const;
	friend std::ostream& operator << (std::ostream& os, const Json& node);
	friend std::ostream& operator << (std::ostream& os, const JsonPtr node);

	JsonPtr getNodeByKey(const std::string& key);
	JsonPtr getNodeByIndex(int index);
	JsonPtr getParentNode(const std::string& path, const std::string& delim, std::string& lastSection, bool& nodeTypeError, bool& nodeNotFound);
	
	//-- return nullptr if node type miss match in the path.
	//-- If target existed, just return the node simply.
	//-- If target isn't exist, just create a uninited node, and return.
	//-- If throwable, throw FpnnJsonNodeTypeMissMatchError.
	JsonPtr createNode(const std::string& path, const std::string& delim, bool throwable);

	// bool dictAddBool(const std::string& key, bool value)                   { return addBasicValue(key, value); }
	// bool dictAddReal(const std::string& key, double value)                 { return addBasicValue(key, value); }
	// bool dictAddInt(const std::string& key, intmax_t value)                { return addBasicValue(key, value); }
	// bool dictAddString(const std::string& key, const std::string& value)   { return addBasicValue(key, value); }

	// bool dictAddNull(const std::string& key);
	// JsonPtr dictAddArray(const std::string& key);
	JsonPtr dictAddObject(const std::string& key);

	Json& operator = (const Json&) = delete;
	Json(const Json&) = delete;

	Json(const std::string& value): _type(JSON_String), _data(new std::string(value)) {}
	Json(const char* value): _type(JSON_String), _data(new std::string(value ? value : "")) {}
	Json(intmax_t value): _type(JSON_Integer), _data(new intmax_t(value)) {}
	Json(uintmax_t value): _type(JSON_UInteger), _data(new uintmax_t(value)) {}
	Json(long double value): _type(JSON_Real), _data(new double((double)value)) {}
	Json(double value): _type(JSON_Real), _data(new double(value)) {}
	Json(bool value): _type(JSON_Boolean), _data(value ? (void*)1 : 0) {}

	//---------- utils functions : Arrays -------------//
	bool pushNode(JsonPtr node);	//-- If node type miss match, return false.

	//---------- utils functions : Objects -------------//
	bool addNode(const std::string& key, JsonPtr node);   //-- If node type miss match, or key exist, return false.

	template<typename TYPE>
	inline bool addBasicValue(const std::string& key, TYPE value)
	{
		//JsonPtr node(new Json(value));
		JsonPtr node(new Json());
		(*node) = value;
		return addNode(key, node);
	}

	friend class JsonParser;

public:
	Json(): _type(JSON_Uninit), _data(NULL) {}
	~Json() { clean(); }

	static JsonPtr parse(const char* data);
	//static JsonPtr parse(const char* data, int size);  //-- will add future
	//static JsonPtr parse(const std::string& data);     //-- will add future
	//static JsonPtr parse(const IChainBuffer& data);    //-- will add future
	//static JsonPtr parse(const IChainBuffer* data);    //-- will add future
	//static JsonPtr parse(IChainBufferPtr data);        //-- will add future

	std::string str();

	//==============================[ Set values ]==============================//
	//-- Old value & sub-nodes will be dropped, and changed node type as the value's type.
	//-- Following function will not throw any exception.

	//-------------------[ basic set values ]-------------------//
	void setNull();
	void setBool(bool value);
	void setInt(intmax_t value);
	void setUInt(uintmax_t value);
	void setReal(double value);
	void setString(const char* value);
	void setString(const std::string& value);
	void setArray();		//-- empty array
	void setDict();			//-- empty dict

	Json& operator = (bool value)               { setBool(value);   return *this; }
	Json& operator = (double value)             { setReal(value);   return *this; }
	Json& operator = (long double value)        { setReal(value);   return *this; }
	Json& operator = (const char* value)        { setString(value); return *this; }
	Json& operator = (const std::string& value) { setString(value); return *this; }

	Json& operator = (short value)              { setInt(value);  return *this; }
	Json& operator = (unsigned short value)     { setUInt(value); return *this; }
	Json& operator = (int value)                { setInt(value);  return *this; }
	Json& operator = (unsigned int value)       { setUInt(value); return *this; }
	Json& operator = (long value)               { setInt(value);  return *this; }
	Json& operator = (unsigned long value)      { setUInt(value); return *this; }
	Json& operator = (long long value)          { setInt(value);  return *this; }
	Json& operator = (unsigned long long value) { setUInt(value); return *this; }

	//-------------------[ Advanced set values ]-------------------//
	//-- Please refer the inc file.

	//==============================[ Add Array members ]==============================//
	//-- for array
	//-- Following functions will throw FpnnJsonNodeTypeMissMatchError exception.

	//-------------------[ basic push values ]-------------------//
	//-- If current node's type is not array, will return false or nullptr.
	bool push(bool value)                     { return pushBool(value); }
	bool push(double value)                   { return pushReal(value); }
	bool push(long double value)              { return pushReal(value); }
	bool push(const char* value)              { return pushString(value); }
	bool push(const std::string& value)       { return pushString(value); }

	bool push(short value)                    { return pushInt (value); }
	bool push(unsigned short value)           { return pushUInt(value); }
	bool push(int value)                      { return pushInt (value); }
	bool push(unsigned int value)             { return pushUInt(value); }
	bool push(long value)                     { return pushInt (value); }
	bool push(unsigned long value)            { return pushUInt(value); }
	bool push(long long value)                { return pushInt (value); }
	bool push(unsigned long long value)       { return pushUInt(value); }

	bool pushNull();
	bool pushBool(bool value)                    { JsonPtr node(new Json(value)); return pushNode(node); }
	bool pushReal(double value)                  { JsonPtr node(new Json(value)); return pushNode(node); }
	bool pushInt(intmax_t value)                 { JsonPtr node(new Json(value)); return pushNode(node); }
	bool pushUInt(uintmax_t value)               { JsonPtr node(new Json(value)); return pushNode(node); }
	bool pushString(const char* value)           { JsonPtr node(new Json(value)); return pushNode(node); }
	bool pushString(const std::string& value)    { JsonPtr node(new Json(value)); return pushNode(node); }
	JsonPtr pushArray();		//-- point added array node.
	JsonPtr pushDict();		//-- point added dict node.

	//-------------------[ Advanced push values ]-------------------//
	//-- Please refer the inc file.


	//-------------------[ Pathlized push values ]-------------------//
	//-- for array
	//-- ** The following functions will throw exception: FpnnJsonNodeTypeMissMatchError **
	//-- If target existed, this operation will change the target node to array type implicitly.
	void push(const std::string& path, bool value, const std::string& delim = "./")               { pushBool(path, value, delim); }
	void push(const std::string& path, double value, const std::string& delim = "./")             { pushReal(path, value, delim); }
	void push(const std::string& path, long double value, const std::string& delim = "./")        { pushReal(path, value, delim); }
	void push(const std::string& path, const char* value, const std::string& delim = "./")        { pushString(path, value, delim); }
	void push(const std::string& path, const std::string& value, const std::string& delim = "./") { pushString(path, value, delim); }

	void push(const std::string& path, short value, const std::string& delim = "./")              { pushInt (path, value, delim); }
	void push(const std::string& path, unsigned short value, const std::string& delim = "./")     { pushUInt(path, value, delim); }
	void push(const std::string& path, int value, const std::string& delim = "./")                { pushInt (path, value, delim); }
	void push(const std::string& path, unsigned int value, const std::string& delim = "./")       { pushUInt(path, value, delim); }
	void push(const std::string& path, long value, const std::string& delim = "./")               { pushInt (path, value, delim); }
	void push(const std::string& path, unsigned long value, const std::string& delim = "./")      { pushUInt(path, value, delim); }
	void push(const std::string& path, long long value, const std::string& delim = "./")          { pushInt (path, value, delim); }
	void push(const std::string& path, unsigned long long value, const std::string& delim = "./") { pushUInt(path, value, delim); }

	void pushNull(const std::string& path, const std::string& delim = "./");
	void pushBool(const std::string& path, bool value, const std::string& delim = "./");
	void pushReal(const std::string& path, double value, const std::string& delim = "./");
	void pushInt(const std::string& path, intmax_t value, const std::string& delim = "./");
	void pushUInt(const std::string& path, uintmax_t value, const std::string& delim = "./");
	void pushString(const std::string& path, const char* value, const std::string& delim = "./");
	void pushString(const std::string& path, const std::string& value, const std::string& delim = "./");
	JsonPtr pushArray(const std::string& path, const std::string& delim = "./");			//-- point pushed array node.
	JsonPtr pushDict(const std::string& path, const std::string& delim = "./");			//-- point pushed dict node.


	//==============================[ Merge data to Dict or Array Node ]==============================//
	

	//-------------------[ Advanced merge values ]-------------------//
	//-- Please refer the inc file.

	//-------------------[ Pathlized merge values ]-------------------//
	//-- Please refer the inc file.

	//==============================[ Add data to Dict Node ]==============================//
	//-- for dict
	//-- the following functions will throw exception: FpnnJsonNodeTypeMissMatchError or FpnnJosnNodeExistError

	void add(const std::string& path, bool value, const std::string& delim = "./")                  { addBool(path, value, delim); }
	void add(const std::string& path, double value, const std::string& delim = "./")                { addReal(path, value, delim); }
	void add(const std::string& path, long double value, const std::string& delim = "./")           { addReal(path, value, delim); }
	void add(const std::string& path, const char* value, const std::string& delim = "./")           { addString(path, value, delim); }
	void add(const std::string& path, const std::string& value, const std::string& delim = "./")    { addString(path, value, delim); }

	void add(const std::string& path, short value, const std::string& delim = "./")                 { addInt (path, value, delim); }
	void add(const std::string& path, unsigned short value, const std::string& delim = "./")        { addUInt(path, value, delim); }
	void add(const std::string& path, int value, const std::string& delim = "./")                   { addInt (path, value, delim); }
	void add(const std::string& path, unsigned int value, const std::string& delim = "./")          { addUInt(path, value, delim); }
	void add(const std::string& path, long value, const std::string& delim = "./")                  { addInt (path, value, delim); }
	void add(const std::string& path, unsigned long value, const std::string& delim = "./")         { addUInt(path, value, delim); }
	void add(const std::string& path, long long value, const std::string& delim = "./")             { addInt (path, value, delim); }
	void add(const std::string& path, unsigned long long value, const std::string& delim = "./")    { addUInt(path, value, delim); }

	void addBool(const std::string& path, bool value, const std::string& delim = "./");
	void addReal(const std::string& path, double value, const std::string& delim = "./");
	void addInt(const std::string& path, intmax_t value, const std::string& delim = "./");
	void addUInt(const std::string& path, uintmax_t value, const std::string& delim = "./");
	void addString(const std::string& path, const char* value, const std::string& delim = "./");
	void addString(const std::string& path, const std::string& value, const std::string& delim = "./");

	void addNull(const std::string& path, const std::string& delim = "./");
	JsonPtr addArray(const std::string& path, const std::string& delim = "./");
	JsonPtr addDict(const std::string& path, const std::string& delim = "./");

	//-------------------[ Advanced add values ]-------------------//
	//-- Please refer the inc file.

	//==============================[ remove sub-nodes ]==============================//
	//-- If node is not array type, return false;
	//-- if index out of range, just return true simply.
	bool remove(int index);

	//-- If node in path is not object type, return false;
	//-- if key or node in path is not exist, just return true simply.
	bool remove(const std::string& path, const std::string& delim = "./");

	//==============================[ member checking ]==============================//
	inline bool isNull() const { return _type == JSON_Null; }
	inline enum ElementType type() const { return _type; }

	//-- isNull(): return true only the target exist and which is null.
	bool isNull(const std::string& path, const std::string& delim = "./") noexcept;
	bool exist(const std::string& path, const std::string& delim = "./") noexcept { return (getNode(path, delim) != nullptr); }
	enum ElementType type(const std::string& path, const std::string& delim = "./");

	//==============================[ fetch members ]==============================//
	//------------ get part ------------//
	bool getBool(bool dft = false) const;
	intmax_t getInt(intmax_t dft = 0) const;
	uintmax_t getUInt(uintmax_t dft = 0) const;
	double getReal(double dft = 0.0) const;
	std::string getString(const std::string& dft = std::string()) const;

	const std::list<JsonPtr> * const getList() const;
	const std::map<std::string, JsonPtr> * const getDict() const;

	//------------ want part ------------//
	operator bool()        const { return wantBool(); }
	operator float()       const { return (float)wantReal(); }
	operator double()      const { return wantReal(); }
	operator long double() const { return wantReal(); }
	operator std::string() const { return wantString(); }

	operator char()               const { return (char)              wantInt (); }
	operator unsigned char()      const { return (unsigned char)     wantUInt(); }
	operator short()              const { return (short)             wantInt (); }
	operator unsigned short()     const { return (unsigned short)    wantUInt(); }
	operator int()                const { return (int)               wantInt (); }
	operator unsigned int()       const { return (unsigned int)      wantUInt(); }
	operator long()               const { return (long)              wantInt (); }
	operator unsigned long()      const { return (unsigned long)     wantUInt(); }
	operator long long()          const { return (long long)         wantInt (); }
	operator unsigned long long() const { return (unsigned long long)wantUInt(); }

	bool wantBool() const;
	intmax_t wantInt() const;
	uintmax_t wantUInt() const;
	double wantReal() const;
	std::string wantString() const;

	/*
		* Deprecated !!! *

		All want???Vector(), want???Dict() are deprecated!!!

		Please using:

			wantVector<???>(), wantList<???>(), wantSet<???>(), ..., wantDict<???>()

			Please refer "FPJson.Enhancement.inc.h" for all new interfaces.
	*/
	std::vector<bool> wantBoolVector() const;
	std::vector<double> wantRealVector() const;
	std::vector<intmax_t> wantIntVector() const;
	std::vector<std::string> wantStringVector() const;

	std::map<std::string, bool> wantBoolDict() const;
	std::map<std::string, double> wantRealDict() const;
	std::map<std::string, intmax_t> wantIntDict() const;
	std::map<std::string, std::string> wantStringDict() const;

	//-------------------[ fetch members by path ]-------------------//
	//------------ get part ------------//
	bool getBool(const std::string& path, bool dft = false, const std::string& delim = "./");
	intmax_t getInt(const std::string& path, intmax_t dft = 0, const std::string& delim = "./");
	uintmax_t getUInt(const std::string& path, uintmax_t dft = 0, const std::string& delim = "./");
	double getReal(const std::string& path, double dft = 0.0, const std::string& delim = "./");
	std::string getStringAt(const std::string& path, const std::string& dft = std::string(), const std::string& delim = "./");

	bool getBool(const char* path, bool dft = false, const std::string& delim = "./")                                  { return getBool(std::string(path), dft, delim); }
	intmax_t getInt(const char* path, intmax_t dft = 0, const std::string& delim = "./")                               { return getInt(std::string(path), dft, delim); }
	uintmax_t getUInt(const char* path, uintmax_t dft = 0, const std::string& delim = "./")                            { return getUInt(std::string(path), dft, delim); }
	double getReal(const char* path, double dft = 0.0, const std::string& delim = "./")                                { return getReal(std::string(path), dft, delim); }
	std::string getStringAt(const char* path, const std::string& dft = std::string(), const std::string& delim = "./") { return getStringAt(std::string(path), dft, delim); }

	JsonPtr getNode(const std::string& path, const std::string& delim = "./");
	const std::list<JsonPtr> * const getList(const std::string& path, const std::string& delim = "./");
	const std::map<std::string, JsonPtr> * const getDict(const std::string& path, const std::string& delim = "./");

	JsonPtr getNode(const char* path, const std::string& delim = "./")                                       { return getNode(std::string(path), delim); }
	const std::list<JsonPtr> * const getList(const char* path, const std::string& delim = "./")              { return getList(std::string(path), delim); }
	const std::map<std::string, JsonPtr> * const getDict(const char* path, const std::string& delim = "./")  { return getDict(std::string(path), delim); }

	//------------ want part ------------//
	bool wantBool(const std::string& path, const std::string& delim = "./");
	intmax_t wantInt(const std::string& path, const std::string& delim = "./");
	uintmax_t wantUInt(const std::string& path, const std::string& delim = "./");
	double wantReal(const std::string& path, const std::string& delim = "./");
	std::string wantString(const std::string& path, const std::string& delim = "./");
	std::string wantStringAt(const std::string& path, const std::string& delim = "./") { return wantString(path, delim); }

	bool wantBool(const char* path, const std::string& delim = "./")            { return wantBool(std::string(path), delim); }
	intmax_t wantInt(const char* path, const std::string& delim = "./")         { return wantInt(std::string(path), delim); }
	uintmax_t wantUInt(const char* path, const std::string& delim = "./")       { return wantUInt(std::string(path), delim); }
	double wantReal(const char* path, const std::string& delim = "./")          { return wantReal(std::string(path), delim); }
	std::string wantString(const char* path, const std::string& delim = "./")   { return wantString(std::string(path), delim); }
	std::string wantStringAt(const char* path, const std::string& delim = "./") { return wantString(std::string(path), delim); }

	//-- If any node in path (except the last node) which node type is not dict/object, will throw exception.
	Json& operator [] (const char* path);
	Json& operator [] (const std::string& path);
	Json& operator [] (int index);

	/*
		* Deprecated !!! *

		All want???Vector(...), want???Dict(...) are deprecated!!!

		Please using:

			wantVector<???>(...), wantList<???>(...), wantSet<???>(...), ..., wantDict<???>(...)

			Please refer "FPJson.Enhancement.inc.h" for all new interfaces.
	*/
	std::vector<bool> wantBoolVector(const std::string& path, const std::string& delim = "./");
	std::vector<double> wantRealVector(const std::string& path, const std::string& delim = "./");
	std::vector<intmax_t> wantIntVector(const std::string& path, const std::string& delim = "./");
	std::vector<std::string> wantStringVector(const std::string& path, const std::string& delim = "./");

	std::map<std::string, bool> wantBoolDict(const std::string& path, const std::string& delim = "./");
	std::map<std::string, double> wantRealDict(const std::string& path, const std::string& delim = "./");
	std::map<std::string, intmax_t> wantIntDict(const std::string& path, const std::string& delim = "./");
	std::map<std::string, std::string> wantStringDict(const std::string& path, const std::string& delim = "./");


	std::vector<bool> wantBoolVector(const char* path, const std::string& delim = "./")          { return wantBoolVector(std::string(path), delim); }
	std::vector<double> wantRealVector(const char* path, const std::string& delim = "./")        { return wantRealVector(std::string(path), delim); }
	std::vector<intmax_t> wantIntVector(const char* path, const std::string& delim = "./")       { return wantIntVector(std::string(path), delim); }
	std::vector<std::string> wantStringVector(const char* path, const std::string& delim = "./") { return wantStringVector(std::string(path), delim); }

	std::map<std::string, bool> wantBoolDict(const char* path, const std::string& delim = "./")          { return wantBoolDict(std::string(path), delim); }
	std::map<std::string, double> wantRealDict(const char* path, const std::string& delim = "./")        { return wantRealDict(std::string(path), delim); }
	std::map<std::string, intmax_t> wantIntDict(const char* path, const std::string& delim = "./")       { return wantIntDict(std::string(path), delim); }
	std::map<std::string, std::string> wantStringDict(const char* path, const std::string& delim = "./") { return wantStringDict(std::string(path), delim); }

public:
	#include "FPJson.Enhancement.inc.h"
	#include "FPJson.Enhancement.Extends.inc.h"
};

}

#endif
