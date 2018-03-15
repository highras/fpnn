#include <strings.h>
#include <string.h>
#include "TableRow.h"
using namespace fpnn;
void ROW::create(const vector<string>& row){
	_num_field = row.size();
	_num_long_field = 0;

	uint32_t *lens = (uint32_t*)alloca(_num_field * sizeof(uint32_t));
	if(!lens)
		throw runtime_error("malloc lens error when create ROW");

	size_t length = get_data_len(row, lens);

	uint32_t llen = _num_long_field * sizeof(uint32_t);

	uint32_t head_len = _num_field + llen;

	_data = (char*)malloc(head_len + length);
	if (!_data)
		throw runtime_error("malloc data error when create ROW");

	uint32_t *lhead = (uint32_t*)_data;
	uint8_t *head = (uint8_t*)(_data + llen);
	uint32_t current = head_len;

	for (size_t i = 0; i < _num_field; ++i)
	{
		if (lens[i] < 255){
			*head = lens[i];
		}
		else{
			*head = 255;
			*lhead = lens[i];
			++lhead;
		}
		++head;

		memcpy(_data + current, row[i].data(), row[i].size());
		current += row[i].size();
	}
}

vector<string> ROW::get_data(TABLE* table, const vector<string>& fields){
	vector<uint16_t> field_index = table->get_fields_index(fields);
	return get_data(field_index);
}

vector<string> ROW::get_data(const vector<uint16_t> field_index){
	if (!_data)
		throw runtime_error("Empty Row!!!");

	uint32_t *lhead = (uint32_t*)_data;
	uint32_t llen = _num_long_field * sizeof(uint32_t);
	uint8_t *head = (uint8_t*)(_data + llen);
	size_t n = _num_field + llen;
	size_t k = 0;

	uint32_t *strs = (uint32_t*)alloca(_num_field * sizeof(uint32_t) * 2);
	if(!strs)
		throw runtime_error("Can not alloc str when get data in ROW");

	for (size_t i = 0; i < _num_field; ++i){
		size_t pos = i * 2;
		if (head[i]){
			strs[pos] = n;
			strs[pos + 1] = head[i] < 255 ? head[i] : lhead[k++];
			n += strs[pos + 1];
		}
		else{
			strs[pos] = n;
			strs[pos + 1] = 0;
		}
	}
	vector<string> result;
	for (size_t i = 0; i < field_index.size(); ++i)
	{   
		if (field_index[i] >= _num_field)
			throw runtime_error("invalid field index");
		size_t pos = field_index[i] * 2;
		if (!strs[pos + 1])
			result.push_back("");
		else
			result.push_back(string(_data + strs[pos], strs[pos + 1]));
	}
	return result;
}

size_t ROW::get_data_len(const vector<string>& data, uint32_t *lens){
	size_t len = 0;
	for (int i = 0; i < _num_field; ++i){
		lens[i] = data[i].size();
		if (lens[i] >= 255)
			++_num_long_field;

		len += lens[i];
	}
	return len;
}

void TABLE::re_init(const vector<vector<string> >& qres){
	lock_guard<mutex> lck(_mutex);

	_select_fields = "";
	_fields.clear();
	_int_fields.clear();
	_fields_index.clear();

	if (qres.size() == 0)
		throw runtime_error("Can't get the table struct");

	int index = 0;
	for(size_t i = 0; i < qres.size(); ++i){
		const vector<string>& row = qres[i];

		if(_select_fields.length() > 0) _select_fields += ",";
		_select_fields += row[0];
		_fields.push_back(row[0]);

		_fields_index[row[0] ] = index++;
		if(checkField(row[1])){
			_int_fields.insert(row[0]);
		}   
	}       	
}

vector<uint16_t> TABLE::get_fields_index(const vector<string>& fields){
	vector<uint16_t> fields_index;
	lock_guard<mutex> lck(_mutex);
	for(size_t j = 0; j < fields.size(); ++j){
		uint16_t index = _fields_index.at(fields[j]);//will throw an exception if not exist
		fields_index.push_back(index);
	}
	return fields_index;
}

bool TABLE::checkField(const string& field){
	if(strncasecmp(field.c_str(), "BIT", strlen("BIT")) == 0
			|| strncasecmp(field.c_str(), "TINYINT", strlen("TINYINT")) == 0
			|| strncasecmp(field.c_str(), "BOOL", strlen("BOOL")) == 0
			|| strncasecmp(field.c_str(), "SMALLINT", strlen("SMALLINT")) == 0
			|| strncasecmp(field.c_str(), "MEDIUMINT", strlen("MEDIUMINT")) == 0
			|| strncasecmp(field.c_str(), "INT", strlen("INT")) == 0
			|| strncasecmp(field.c_str(), "BIGINT", strlen("BIGINT")) == 0
			|| strncasecmp(field.c_str(), "FLOAT", strlen("FLOAT")) == 0
			|| strncasecmp(field.c_str(), "DOUBLE", strlen("DOUBLE")) == 0
			|| strncasecmp(field.c_str(), "DEC", strlen("DEC")) == 0)
		return true;
	return false;
}

#ifdef TEST_TABLE_ROW
//g++ -g -DTEST_TABLE_ROW TableRow.cpp -std=c++11
#include <sstream>
#include <iostream>
using namespace std;

int main(int argc, char **argv)
{
	vector<vector<string> > qres;
	vector<string> v;
	v.push_back("fpid");
	v.push_back("bigint(20)");
	qres.push_back(v);
	v.clear();
	v.push_back("add_time");
	v.push_back("timestamp");
	qres.push_back(v);
	v.clear();
	v.push_back("add_ip");
	v.push_back("char(15)");
	qres.push_back(v);
	v.clear();
	v.push_back("idfa");
	v.push_back("varbinary");
	qres.push_back(v);
	v.clear();
	v.push_back("value");
	v.push_back("varchar");
	qres.push_back(v);
	v.clear();
	v.push_back("login");
	v.push_back("date");
	qres.push_back(v);
	v.clear();
	v.push_back("android_id");
	v.push_back("char");
	qres.push_back(v);
	v.clear();
	v.push_back("value2");
	v.push_back("varchar");
	qres.push_back(v);
	v.clear();


	TABLE tab("tablename", "fpid", qres);
	cout<<"select fields:"<<tab.get_select_string()<<endl;

	vector<string> fields;
	fields.push_back("value2");
	fields.push_back("idfa");
	fields.push_back("login");
	fields.push_back("fpid");

	vector<uint16_t> fields_index = tab.get_fields_index(fields);
	for(size_t i = 0; i < fields.size(); ++i){
		cout<<"get field index of " << fields[i] << " : " << fields_index[i] <<endl;
	}

	cout<<"fpid type:"<<tab.isStringField("fpid")<<endl;
	cout<<"value2 type:"<<tab.isStringField("value2")<<endl;
	cout<<"android_id type:"<<tab.isStringField("android_id")<<endl;
	cout<<"idfa type:"<<tab.isStringField("idfa")<<endl;

	v.push_back("password");
	v.push_back("varchar");
	qres.push_back(v);
	v.clear();
	tab.re_init(qres);

	fields.push_back("password");

	cout<<"RE INIT"<<endl;

	cout<<"select fields:"<<tab.get_select_string()<<endl;
	fields_index = tab.get_fields_index(fields);
	for(size_t i = 0; i < fields.size(); ++i){
		cout<<"get field index of " << fields[i] << " : " << fields_index[i] <<endl;
	}
	cout<<"fpid type:"<<tab.isStringField("fpid")<<endl;
	cout<<"value2 type:"<<tab.isStringField("value2")<<endl;
	cout<<"android_id type:"<<tab.isStringField("android_id")<<endl;
	cout<<"idfa type:"<<tab.isStringField("idfa")<<endl;

	cout<< "ROW" << endl;

	vector<string> row;
	row.push_back("89999999998");
	row.push_back("100");
	row.push_back("192.168.100.102");
	row.push_back("ABCEDSFWGESFDSAFDFBA");
	row.push_back("Amazon Aurora 是一个兼容 MySQL 的关系型数据库引擎，结合了高端商用数据库的速度和可用性以及开源数据库的简单性和成本效益。Amazon Aurora 继 MySQL、Oracle、Microsoft SQL Server 和 PostgreSQL 之后，成为第五个可通过 Amazon RDS 提供给客户的数据库引擎。");
	row.push_back("2016-05-18");
	row.push_back("12oiuuusfsfr93850224-242-ojsf21");
	row.push_back("默认情况下，允许客户有总共 40 个 Amazon RDS 数据库实例。在这 40 个实例中，最多 10 个实例可以是“包含许可证”模式下的 Oracle 或 SQL Server 数据库实例。40 个实例全都可用于“BYOL”模式下的 MySQL、Oracle、SQL Server 或 PostgreSQL。如果您的应用程序需要更多数据库实例，可以通过此申请表申请更多数据库实例。");
	row.push_back("I'm a pssword");
	
	ROW mRow(row);
	vector<string> mGet = mRow.get_data(&tab, fields);
	for(size_t i = 0; i < mGet.size(); ++i){
		cout<<mGet[i]<<endl;
	}

	fields.push_back("value");
	cout<<endl<<endl;
	mGet = mRow.get_data(&tab, fields);
	for(size_t i = 0; i < mGet.size(); ++i){
		cout<<mGet[i]<<endl;
	}

	cout<<endl<<endl;
	vector<uint16_t> field_index2 = tab.get_fields_index(fields);
	mGet = mRow.get_data(field_index2);
	for(size_t i = 0; i < mGet.size(); ++i){
		cout<<mGet[i]<<endl;
	}

	return 0;
}

#endif
