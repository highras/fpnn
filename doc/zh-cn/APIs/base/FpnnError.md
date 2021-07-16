## FpnnError

### 介绍

FPNN Framework 内部异常 & 错误码定义。

### 命名空间

	namespace fpnn;

#### FpnnError

	class FpnnError: public std::exception
	{
	public:
		FpnnError(const char *file, const char* fun, int32_t line, int32_t code = 0, const std::string& msg = "");
		virtual ~FpnnError() noexcept;

		virtual FpnnError* clone() const;
		virtual void do_throw() const;

		virtual const char* what() const noexcept;

		const char* file() const noexcept;
		const char* fun() const noexcept;
		int32_t line() const noexcept;
		int32_t code() const noexcept;
		const std::string& message() const noexcept;

		static std::string format(const char *fmt, ...);
	};

FPNN 异常对象。

#### errorCode

具体错误码请参见 [errorCode](../../fpnn-error-code.md)。