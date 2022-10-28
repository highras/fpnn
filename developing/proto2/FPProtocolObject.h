#ifndef FPNN_Protocol_Object_H
#define FPNN_MSGPACK_H

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
#include "FpnnError.h"

namespace fpnn {

/*******************************************************************
  Exception
*******************************************************************/
FPNN_(FpnnLogicError, FpnnProtocolObjectExistError)
FPNN_(FpnnLogicError, FpnnProtocolObjectNotExistError)
FPNN_(FpnnLogicError, FpnnProtocolObjectTypeMissMatchError)

class ProtocolObject;
typedef std::shared_ptr<ProtocolObject> ProtocolObjectPtr;

std::ostream& operator << (std::ostream& os, const ProtocolObject& obj);
std::ostream& operator << (std::ostream& os, const ProtocolObjectPtr obj);

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
class ProtocolObject
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

};

}

#endif