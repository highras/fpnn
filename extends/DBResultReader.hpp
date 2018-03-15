#ifndef Data_Router_Result_Reader_H
#define Data_Router_Result_Reader_H

#include <stdlib.h>
#include <vector>
#include "FpnnError.h"
#include "FPReader.h"

namespace fpnn {
FPNN_(FpnnError, DBResultError)

class DBResultReader
{
	fpnn::FPAReader _reader;
	std::vector<std::string> *_fields;
	std::vector<std::vector<std::string>> *_rows;

	std::vector<int64_t> *_invalidIds;
	std::vector<int64_t> *_failedIntIds;
	std::vector<std::string> *_failedStringIds;

	inline void readFieldsRows()
	{
		if (_rows)
			return;

		_fields = new std::vector<std::string>();
		_rows = new std::vector<std::vector<std::string>>();

		*_fields = _reader.want("fields", std::vector<std::string>());
		*_rows = _reader.want("rows", std::vector<std::vector<std::string>>());
	}

public:
	DBResultReader(const fpnn::FPAnswerPtr& answer): _reader(answer), _fields(NULL), _rows(NULL),
		_invalidIds(NULL), _failedIntIds(NULL), _failedStringIds(NULL)
	{
	}

	~DBResultReader()
	{
		if (_fields)
			delete _fields;

		if (_rows)
			delete _rows;

		if (_invalidIds)
			delete _invalidIds;

		if (_failedIntIds)
			delete _failedIntIds;

		if (_failedStringIds)
			delete _failedStringIds;
	}

	//---------------[ for exception ]-----------------//
	void exception()
	{
		if (_reader.status())
			throw DBResultError("DBRouter", "DBResultReader", __LINE__, code(), ex());
	}
	inline bool isException()
	{
		return _reader.status();
	}

	inline int code() { return (int)_reader.wantInt("code"); }
	inline std::string ex() { return _reader.wantString("ex"); }
	inline std::string raiser() { return _reader.wantString("raiser"); }

	//---------------[ for write options ]-----------------//
	inline int affectedRows() { return (int)_reader.wantInt("affectedRows"); }
	inline int insertId() { return (int)_reader.wantInt("insertId"); }

	//---------------[ for invalied & failed Ids ]-----------------//
	inline const std::vector<int64_t>& invalidIds()
	{
		if (!_invalidIds)
		{
			_invalidIds = new std::vector<int64_t>();
			*_invalidIds = _reader.get("invalidIds", std::vector<int64_t>());
		}
		return *_invalidIds;
	}
	inline const std::vector<int64_t>& failedIds(int)
	{
		if (!_failedIntIds)
		{
			_failedIntIds = new std::vector<int64_t>();
			*_failedIntIds = _reader.get("failedIds", std::vector<int64_t>());
		}
		return *_failedIntIds;
	}
	inline const std::vector<std::string>& failedIds(const std::string&)
	{
		if (!_failedStringIds)
		{
			_failedStringIds = new std::vector<std::string>();
			*_failedStringIds = _reader.get("failedIds", std::vector<std::string>());
		}
		return *_failedStringIds;
	}

	//---------------[ for select & desc options ]-----------------//
	inline const std::vector<std::string>& fields()
	{
		readFieldsRows();
		return *_fields;
	}
	inline int fieldIndex(const std::string &fieldName)
	{
		readFieldsRows();
		for (size_t i = 0; i < _fields->size(); i++)
		{
			if (fieldName == (*_fields)[i])
				return i;
		}
		return -1;
	}

	inline int rowsCount() { readFieldsRows(); return (int)_rows->size(); }
	inline const std::vector<std::string>& row(int index) { readFieldsRows(); return (*_rows)[index]; }
	inline const std::vector<std::vector<std::string> >& rows() { readFieldsRows(); return *_rows; }

	inline const std::string& cell(int rowIndex, int fieldIndex) { readFieldsRows(); return (*_rows)[rowIndex][fieldIndex]; }
	inline const std::string& cell(int rowIndex, const std::string& fieldName) { return cell(rowIndex, fieldIndex(fieldName)); }
	
	inline int64_t intCell(int rowIndex, int fieldIndex) { return atoll(cell(rowIndex, fieldIndex).c_str()); }
	inline int64_t intCell(int rowIndex, const std::string& fieldName) { return atoll(cell(rowIndex, fieldName).c_str()); }
	
	inline double realCell(int rowIndex, int fieldIndex) { return atof(cell(rowIndex, fieldIndex).c_str()); }
	inline double realCell(int rowIndex, const std::string& fieldName) { return atof(cell(rowIndex, fieldName).c_str()); }	
};
}
#endif
