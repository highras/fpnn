# FPNN 代码规范

## C++ 部分

1. 所有智能指针方面的函数，库均使用 std c++

1. 所有concurrency，thread方面的函数，库优先使用 std c++

1. hash map, hash set 请使用 unordered_map, unordered_set

1. 避免编译时候的 warning

1. 避免使用 int，请使用 int32_t, uint64_t 类似的函数

1. 内存优化使用 tcmalloc

1. 除非特殊需要，否则禁止使用 boost

1. 除非特殊需要，否则禁止使用正则表达式和正则匹配

1. 部分语言没有unsigned 类型，设计协议的时候需要考虑

1. 框架与SDK部分，尽可能用系统平台和语言自带的库。未经同意，不得使用第三方库
