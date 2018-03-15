#include "gzpipe.h"
#include "FpnnError.h"
#include <string.h>

using namespace fpnn;

#define PIPE_CHUNK 8192

std::string gzPipe::compress(const std::string& data){
	return compress(data.data(), data.size());
}

std::string gzPipe::compress(const void* data, size_t size){
	z_stream zs;
    int ret;
	unsigned char outbuffer[PIPE_CHUNK];
	std::string outstring;

	memset(&zs, 0, sizeof(zs));

	ret = deflateInit(&zs, Z_BEST_COMPRESSION);
	if (ret != Z_OK)
		throw FPNN_ERROR_CODE_MSG(FpnnEncryptError, FPNN_EC_ZIP_COMPRESS, "deflateInit failed while compressing");

	zs.next_in = (uint8_t*)data;
	zs.avail_in = size;

	do {
        zs.next_out = outbuffer;
        zs.avail_out = PIPE_CHUNK;

        ret = deflate(&zs, Z_FINISH);
		if (ret == Z_STREAM_ERROR) { 
			throw FPNN_ERROR_CODE_MSG(FpnnEncryptError, FPNN_EC_ZIP_COMPRESS, "Exception during zlib compression: (" + std::to_string(ret) + ") " + zs.msg);
		}

		unsigned have = PIPE_CHUNK - zs.avail_out;
		outstring.append((char*)outbuffer, have);
    } while (zs.avail_out == 0);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) { 
		throw FPNN_ERROR_CODE_MSG(FpnnEncryptError, FPNN_EC_ZIP_COMPRESS, "Exception during zlib compression: (" + std::to_string(ret) + ") " + zs.msg);
    }

    return outstring;
}

std::string gzPipe::decompress(const std::string& data){
	return decompress(data.data(), data.size());
}

std::string gzPipe::decompress(const void* data, size_t size){
	z_stream zs;                    
    int ret;
	unsigned char outbuffer[PIPE_CHUNK];
    std::string outstring;

    memset(&zs, 0, sizeof(zs));

    if (inflateInit(&zs) != Z_OK)
		throw FPNN_ERROR_CODE_MSG(FpnnEncryptError, FPNN_EC_ZIP_DECOMPRESS, "inflateInit failed while decompressing.");

    zs.next_in = (uint8_t*)data;
    zs.avail_in = size;

	do {
		zs.next_out = outbuffer;
		zs.avail_out = PIPE_CHUNK;

		ret = inflate(&zs, 0);
		if (ret == Z_STREAM_ERROR || ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) { 
			throw FPNN_ERROR_CODE_MSG(FpnnEncryptError, FPNN_EC_ZIP_DECOMPRESS, "Exception during zlib decompression: (" + std::to_string(ret) + ") " + zs.msg);
		}

		unsigned have = PIPE_CHUNK - zs.avail_out;
		outstring.append((char*)outbuffer, have);

	} while(zs.avail_out == 0);

	inflateEnd(&zs);

	if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
		throw FPNN_ERROR_CODE_MSG(FpnnEncryptError, FPNN_EC_ZIP_DECOMPRESS, "Exception during zlib decompression: (" + std::to_string(ret) + ") " + zs.msg);
	}

	return outstring;
}


#ifdef TEST_GZPIPE
//g++ -g -DTEST_GZPIPE gzpipe.cpp -std=c++11 -lz
#include <sstream>
#include <iostream>
#include <fstream> 
using namespace std;
using namespace gzPipe;

int main(int argc, char **argv)
{
	string data;

	for(size_t i = 0; i < 1024*1024+13; ++i)
		//data.push_back((rand()%255));
		data.push_back('A');

	string cdata = compress(data);

	string ddata = decompress(cdata);

	if(data != ddata){
		cout<<"Not Same" <<endl;
	}
	else
		cout<<"Same"<<endl;

	//write to file
	ofstream outfile ("odata", std::ofstream::binary);
	outfile.write(data.data(), data.size());
	outfile.close();

	outfile.open("cdata.gz", std::ofstream::binary);
	outfile.write(cdata.data(), cdata.size());
	outfile.close();

	return 0;

}

/*

<?php
$str = file_get_contents("odata");
$compressed   = gzcompress($str, 9); 
file_put_contents("phpcdata.gz", $compressed);

$str = file_get_contents("cdata.gz");
$uncompressed = gzuncompress($compressed);
file_put_contents("phpodata", $uncompressed);
?> 
*/
#endif
