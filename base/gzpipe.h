#ifndef FPNN_gzpipe_H
#define FPNN_gzpipe_H

#include <string>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <zlib.h>

namespace fpnn {
namespace gzPipe{

	std::string compress(const std::string& data);
	std::string compress(const void* data, size_t size);

	std::string decompress(const std::string& data);
	std::string decompress(const void* data, size_t size);

}
}

#endif
