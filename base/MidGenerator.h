#ifndef MID_GEN_H
#define MID_GEN_H
#include <atomic>
#include <string>

class MidGenerator{
    public:
		//取ip4的最后一位,这样一个网段内部产生的mid不会重复
		//注意::一秒可以最多可以产生千万的不重复的mid
		static void init(const std::string& ip4 = "127.0.0.1");
		static void init(int32_t rand = 0);
        static int64_t genMid();

    private:
		static std::atomic<uint32_t> _sn;
		static int32_t _pip;
};

#endif
