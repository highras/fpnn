#include "DisassembleCommand.h"
#include "CommandProcessor.h"

bool CommandProcessor::registerCmd(const std::string& cmdLeadingHeader, std::function<void (int paramsBeginIdx, const std::vector<std::string>& cmd)> func)
{
	CmdExecutorPtr executor(new std::function<void (int paramsBeginIdx, const std::vector<std::string>& cmd)>(std::move(func)));
	return registerCmd(cmdLeadingHeader, executor);
}

bool CommandProcessor::registerCmd(const std::string& cmdLeadingHeader, CmdExecutorPtr executor)
{
	std::vector<std::string> leadingHeaders = DisassembleCommand::disassemble(cmdLeadingHeader);

	CmdNode* curNode = &_root;
	for (size_t i = 0; i < leadingHeaders.size(); i++)
	{
		auto iter = curNode->subMap.find(leadingHeaders[i]);
		if (iter == curNode->subMap.end())
		{
			CmdNodePtr node(new CmdNode());
			curNode->subMap[leadingHeaders[i]] = node;
			curNode = node.get();
		}
		else
			curNode = iter->second.get();
	}

	if (curNode->executor)
		return false;

	curNode->executor = executor;
	return true;
}

bool CommandProcessor::execute(const std::vector<std::string>& cmd)
{
	CmdExecutorPtr executor;

	int beginIdx = 0;
	CmdNode* curNode = &_root;
	for (size_t i = 0; i < cmd.size(); i++)
	{
		auto iter = curNode->subMap.find(cmd[i]);
		if (iter != curNode->subMap.end())
		{
			beginIdx += 1;
			curNode = iter->second.get();
		}
		else
			break;
	}

	if (curNode->executor)
	{
		(*(curNode->executor))(beginIdx, cmd);
		return true;
	}
	else
		return false;
}

bool CommandProcessor::execute(const std::string& cmd)
{
	std::vector<std::string> cmdSections = DisassembleCommand::disassemble(cmd);
	return execute(cmdSections);
}
