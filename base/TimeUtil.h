#ifndef FPNN_TimeUtil_H
#define FPNN_TimeUtil_H
#include <string>

namespace fpnn {
namespace TimeUtil{

	//Fast but less precision
	//All not accurate
	
	//For HTTP Date Format
	std::string getTimeRFC1123();

	//example: "yyyy-MM-dd HH:mm:ss"
	std::string getDateTime();
	std::string getDateTime(int64_t t);
	
	//example: "yyyy-MM-dd-HH-mm-ss"
	//example: "yyyy/MM/dd/HH/mm/ss"
	std::string getTimeStr(char sep = '-');
	std::string getTimeStr(int64_t t, char sep = '-');

	//example: "yyyy-MM-dd HH:mm:ss,SSS"
	std::string getDateTimeMS(); 
	std::string getDateTimeMS(int64_t t); 
}
}
#endif
