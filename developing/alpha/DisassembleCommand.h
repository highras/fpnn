#ifndef Disassemble_Command_h
#define Disassemble_Command_h

#include <string>
#include <vector>

class DisassembleCommand
{
	std::vector<std::string> _elem;
	size_t _cmdLen;
	size_t _idx;
	char* _pos;

private:
	DisassembleCommand(const char* begin, size_t len);

	void fetchString();
	bool fetchQuotString();
	bool fetchBlock();
	bool disassemble();

public:
	static std::vector<std::string> disassemble(const char* begin, size_t len);
	static std::vector<std::string> disassemble(const std::string& cmd)
	{
		return disassemble(cmd.data(), cmd.length());
	}
};

#endif