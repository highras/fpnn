#ifndef TableRow_h_
#define TableRow_h_
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>

using namespace std;
namespace fpnn {
class TABLE;

class ROW{
		uint16_t _num_field;
		uint16_t _num_long_field;
		char* _data;
	public:
		ROW(const vector<string>& row) { create(row); }
		~ROW() { if(_data) free(_data); }
	public:
		void create(const vector<string>& row);

		vector<string> get_data(const vector<uint16_t> field_index);
		vector<string> get_data(TABLE* table, const vector<string>& fields);

		size_t num_field() const { return _num_field; }
	private:
		size_t get_data_len(const vector<string>& data, uint32_t *lens);

};

class TABLE{
	string _table;
	string _split_key;
	string _select_fields;
	vector<string> _fields;
	unordered_set<string> _int_fields;
	unordered_map<string, uint16_t> _fields_index;
	
	mutex _mutex;
public:
	TABLE(const string& table, const string& split, const vector<vector<string> >& qres){
		_table = table;
		_split_key = split;
		re_init(qres);
	}
	~TABLE() {}

public:
	string get_select_string() {
		lock_guard<mutex> lck(_mutex);
		return _select_fields;
	}
	string get_key_name() {
		return _split_key;
	}
	string get_table_name() {
		return _table;
	}

	const std::vector<std::string>& get_fields_name() { return _fields; }

	void re_init(const vector<vector<string> >& qres);

	vector<uint16_t> get_fields_index(const vector<string>& fields);

	bool isStringField(const string& field){
		lock_guard<mutex> lck(_mutex);
		return (_int_fields.find(field) == _int_fields.end());
	}

private:
	bool checkField(const string& field);
};

typedef shared_ptr<ROW> ROWPtr;
typedef shared_ptr<TABLE> TABLEPtr;
}
#endif
