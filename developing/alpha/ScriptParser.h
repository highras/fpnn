#ifndef Script_Parser_h
#define Script_Parser_h

#include <string>
#include <memory>
#include <stack>
#include <vector>

struct CmdBlock;
typedef std::shared_ptr<struct CmdBlock> CmdBlockPtr;

struct CmdBlock
{
	enum CmdBlockType
	{
		CBT_If,
		CBT_Forever,
		CBT_While,
		CBT_CountLoop,
		CBT_Normal
	};

	enum CmdBlockType blockType;
	int loopCount;
	std::string command;	//-- normal cmd or if condition cmd.
	bool wantBlockSign;
	bool wantTrueBlock;
	bool wantElseFlag;
	bool wantFalseBlock;

	std::vector<CmdBlockPtr> commBlockChains;
	std::vector<CmdBlockPtr> trueBlockChains;
	std::vector<CmdBlockPtr> falseBlockChains;

	CmdBlock(enum CmdBlockType type, const std::string& cmd = std::string());
	// void printStatus(); //-- debug using.
};

class ScriptParser
{
	CmdBlockPtr _cmdBlock;
	std::stack<CmdBlockPtr> _cmdBlockStack;

public:
	static const std::string conditionHeadString;

private:
	ScriptParser();
	bool praseScript(const std::string& scriptFilePath);
	bool parseLine(std::string& cmd, int lineNo);
	bool beginBlock(int lineNo);
	bool finishBlock(int lineNo);
	bool checkFinishBlock(int lineNo);
	bool addConditionCmdBlock(CmdBlockPtr block, int lineNo);
	bool parseForever(std::vector<std::string>& cmd, int lineNo);
	bool parseCountLoop(std::vector<std::string>& cmd, int lineNo);
	bool parseWhileLoop(std::vector<std::string>& cmd, int lineNo);
	bool parseIf(std::vector<std::string>& cmd, int lineNo);
	bool parseElse(std::vector<std::string>& cmd, int lineNo);
	bool fillCmd(const std::string& cmd, int lineNo);

public:
	static CmdBlockPtr parse(const std::string& scriptFilePath);
	static void showScriptGrammars();
	static void showScriptStruct(CmdBlockPtr cmd, int depth);
	static void showScriptStruct(const std::string& scriptFilePath);
};

#endif