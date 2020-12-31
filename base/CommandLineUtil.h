#ifndef FPNN_Command_Line_Parser_h
#define FPNN_Command_Line_Parser_h

#include <map>
#include <string>
#include <stdint.h>
#include <vector>

namespace fpnn
{

//======================================//
//-- For parseing command line params --//
//======================================//
/*
	Sign can only following '-' or '--'.
	If sign has a value, it CANNOT beignning with '-' or '--'.
	DO NOT support a flag (sign without value) front of rest params list. 
*/
class CommandLineParser
{
	static std::map<std::string, std::string> _recognizedParams;
	static std::vector<std::string> _unrecognizedParams;

public:
	static void init(int argc, const char* const * argv, int beginIndex = 1);

	static std::string getString(const std::string& sign, const std::string& dft = std::string());
	static intmax_t getInt(const std::string& sign, intmax_t dft = 0);
	static bool getBool(const std::string& sign, bool dft = false);
	static double getReal(const std::string& sign, double dft = 0.0);
	static bool exist(const std::string& sign);

	static std::vector<std::string> getRestParams();
};

}

#endif
