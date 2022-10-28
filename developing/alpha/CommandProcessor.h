#ifndef FPNN_Command_Processor_h
#define FPNN_Command_Processor_h

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <functional>

typedef std::function<void (int paramsBeginIdx, const std::vector<std::string>& cmd)> CmdExecutor;
typedef std::shared_ptr<CmdExecutor> CmdExecutorPtr;

class CommandProcessor
{
private:
	struct CmdNode;
	typedef std::shared_ptr<struct CmdNode> CmdNodePtr;

	struct CmdNode
	{
		std::map<std::string, CmdNodePtr> subMap;
		CmdExecutorPtr executor;
	};

	CmdNode _root;

public:
	bool registerCmd(const std::string& cmdLeadingHeader, std::function<void (int paramsBeginIdx, const std::vector<std::string>& cmd)> func);
	bool registerCmd(const std::string& cmdLeadingHeader, CmdExecutorPtr executor);
	bool execute(const std::vector<std::string>& cmd);
	bool execute(const std::string& cmd);
};

#endif
