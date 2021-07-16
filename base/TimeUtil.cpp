#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include "msec.h"
#include "TimeUtil.h"

using namespace fpnn;

std::string TimeUtil::getDateStr(int64_t t, char sep){
	char buff[32] = {0};
	struct tm timeInfo;
	time_t timeValue = (time_t)t;
	struct tm *tmT = localtime_r(&timeValue, &timeInfo);
	if (tmT){
		snprintf(buff, sizeof(buff), "%04d%c%02d%c%02d",
				tmT->tm_year+1900, sep, tmT->tm_mon+1, sep, tmT->tm_mday);
	}
	return std::string(buff);
}

std::string TimeUtil::getDateStr(char sep){
    int64_t t = slack_real_sec();
	return TimeUtil::getDateStr(t, sep);
}

std::string TimeUtil::getDateHourStr(int64_t t, char sep){
	char buff[32] = {0};
	struct tm timeInfo;
	time_t timeValue = (time_t)t;
	struct tm *tmT = localtime_r(&timeValue, &timeInfo);
	if (tmT){
		snprintf(buff, sizeof(buff), "%04d%c%02d%c%02d%c%02d",
				tmT->tm_year+1900, sep, tmT->tm_mon+1, sep, tmT->tm_mday, sep, tmT->tm_hour);
	}
	return std::string(buff);
}

std::string TimeUtil::getDateHourStr(char sep){
    int64_t t = slack_real_sec();
	return TimeUtil::getDateHourStr(t, sep);
}

std::string TimeUtil::getTimeStr(int64_t t, char sep){
	char buff[32] = {0};
	struct tm timeInfo;
	time_t timeValue = (time_t)t;
	struct tm *tmT = localtime_r(&timeValue, &timeInfo);
	if (tmT){
		snprintf(buff, sizeof(buff), "%04hd%c%02hhd%c%02hhd%c%02hhd%c%02hhd%c%02hhd",
				(short)(tmT->tm_year+1900), sep, (int8_t)(tmT->tm_mon+1), sep, (int8_t)(tmT->tm_mday), sep,
				(int8_t)(tmT->tm_hour), sep, (int8_t)(tmT->tm_min), sep, (int8_t)(tmT->tm_sec));
	}
	return std::string(buff);
}

std::string TimeUtil::getTimeStr(char sep){
    int64_t t = slack_real_sec();
	return TimeUtil::getTimeStr(t, sep);
}

std::string TimeUtil::getDateTime(int64_t t){
	char buff[32] = {0};

	struct tm timeInfo;
	time_t timeValue = (time_t)t;
	struct tm *tmT = localtime_r(&timeValue, &timeInfo);
	if (tmT)
		strftime(buff, sizeof(buff), "%Y-%m-%d %T", tmT);
	return std::string(buff);
}

std::string TimeUtil::getDateTime(){
    int64_t t = slack_real_sec();
	return TimeUtil::getDateTime(t);
}

std::string TimeUtil::getDateTimeMS(int64_t t){
    char buff[40] = {0};
	time_t sec = t / 1000;
	uint32_t msec = t % 1000;
	struct tm timeInfo;
	struct tm *tmT = localtime_r(&sec, &timeInfo);
	if (tmT)
		snprintf(buff, sizeof(buff), "%04d-%02d-%02d %02d:%02d:%02d,%03d",
				tmT->tm_year+1900, tmT->tm_mon+1, tmT->tm_mday,
				tmT->tm_hour, tmT->tm_min, tmT->tm_sec, msec);
	return std::string(buff);
}

std::string TimeUtil::getDateTimeMS(){
    int64_t t = slack_real_msec();
	return TimeUtil::getDateTimeMS(t);
}

std::string TimeUtil::getTimeRFC1123(){
	static const char* Days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
	static const char* Months[] = {"Jan","Feb","Mar", "Apr", "May", "Jun", "Jul","Aug", "Sep", "Oct","Nov","Dec"};
	char buff[32] = {0};

	time_t timeValue = (time_t)slack_real_sec();
	struct tm brokenTM;
	tm* broken_t = gmtime_r(&timeValue, &brokenTM);

	snprintf(buff, sizeof(buff), "%s, %d %s %d %d:%d:%d GMT",
			Days[broken_t->tm_wday], broken_t->tm_mday, Months[broken_t->tm_mon],
			broken_t->tm_year + 1900,
			broken_t->tm_hour,broken_t->tm_min,broken_t->tm_sec);
	return std::string(buff);
}
