#include "FunCarpSequence.h"
#include "crc.h"
#include "crc64.h"
#include "bit.h"
#include <algorithm>
#include <cassert>

#include <iostream>

using namespace fpnn;

FunCarpSequence::FunCarpSequence(const std::vector<std::string>& server_ids, uint32_t keymask)
{
    _mask = keymask ? round_up_power_two(keymask) - 1 : UINT32_MAX;
    uint64_t members[server_ids.size()] ;
    for(size_t i=0; i < server_ids.size(); ++i){
        *(members+i) = crc64_checksum_cstr(server_ids[i].c_str());
	//std::cout << i << ":" << members[i] << std::endl;
    }
    _carp = carp_create(members, server_ids.size(), NULL);
    //assert(_carp);
    
}

FunCarpSequence::FunCarpSequence(const std::vector<std::string>& server_ids, const std::vector<uint32_t>& weights, uint32_t keymask)
{
    _mask = keymask ? round_up_power_two(keymask) - 1 : UINT32_MAX;
    size_t num = std::min(server_ids.size(),weights.size() );
    uint64_t members[num];
    for(size_t i=0; i < num; ++i){
        *(members+i) = crc64_checksum_cstr(server_ids[i].c_str());
    }    
    uint32_t weight_arr[num];
    for(size_t i=0; i < num; ++i){
        *(weight_arr+i) = weights[i];
    }
    _carp = carp_create_with_weight(members, weight_arr, num, NULL);
    //assert(_carp);
}

FunCarpSequence::~FunCarpSequence()
{
    carp_destroy(_carp);
}

int FunCarpSequence::which(const char* key)
{
    uint32_t keyhash = crc32_checksum_cstr(key);
    keyhash = keyhash & _mask;
    return carp_which(_carp, keyhash);
}

int FunCarpSequence::which(const std::string& key)
{
    return which(key.c_str());
}

int FunCarpSequence::which(const int64_t key)
{
    char buf[32] = {0};
#ifdef __APPLE__
    sprintf(buf,"%lld", key);
#else
    sprintf(buf,"%ld", key);
#endif

    return which(buf);
}

size_t FunCarpSequence::sequence(const char* key, size_t num, std::vector<size_t>& seq_vec)
{
    uint32_t keyhash = crc32_checksum_cstr(key);
    int* seqs = new int[num];
    carp_sequence(_carp, keyhash & _mask, seqs, num);
    seq_vec.resize(num);
    for(size_t i=0; i<num; ++i){
        seq_vec[i] = seqs[i];
    }
    delete[] seqs;
    return num;    
}

size_t FunCarpSequence::sequence(const std::string& key, size_t num, std::vector<size_t>& seq_vec)
{
    return sequence(key.c_str(), num, seq_vec);
}

size_t FunCarpSequence::sequence(const int64_t key, size_t num, std::vector<size_t>& seq_vec)
{
    char buf[32] = {0};
#ifdef __APPLE__
    sprintf(buf,"%lld", key);
#else
    sprintf(buf,"%ld", key);
#endif
    return sequence(buf, num, seq_vec);
}

#ifdef TEST_FUNCARP
#include <sstream> 
#include <iostream>
#include <sys/time.h>
using namespace std;



//#define SIZE	25
#define NUM	254

int main(int argc, char **argv)
{
	int i;
	int count[NUM] = {0};
	vector<string> servers; servers.reserve(NUM);
        vector<uint32_t> weights(NUM);
	struct timeval tv1, tv2;
	gettimeofday(&tv1, NULL);	
	for (i = 0; i < NUM ; ++i){
            stringstream ss;
            ss << "10.1.1." << i << ":11211";
            servers.push_back(ss.str());
            //weights[i] = i % 2 ? 1 : 2;
            weights[i] = 1 ;
	}
        FunCarpSequence seq (servers, weights);
	gettimeofday(&tv2, NULL);	
	unsigned long long used_time1 = 1000000*(tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec);  

	gettimeofday(&tv1, NULL);	
	for (i = 0; i < 1024 * 1024; ++i)
	{

            //stringstream ss; ss << i;
            //vector<size_t> result;
            //seq.sequence(ss.str(), 5, result);
            //count[result[0]]++;
            ++count[seq.which(i+10000)];
	}
	gettimeofday(&tv2, NULL);	
	unsigned long long used_time2 = 1000000*(tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec);  
	
	for (i = 0; i < NUM; ++i)
		cout << i << "\t" << count[i] << endl;
	cout << "used time for create sequence:" << used_time1 << " us" << endl;
	cout << "used time for 1M keys:" << used_time2/1000 << " ms" << endl;
	return 0;
}

#endif
