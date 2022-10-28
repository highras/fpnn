#include <ctype.h>
#include <iostream>
#include "FileSystemUtil.h"
#include "StringUtil.h"
#include "DisassembleCommand.h"
#include "utilTools.h"
#include "ScriptParser.h"

using namespace std;
using namespace fpnn;

CmdBlock::CmdBlock(enum CmdBlockType type, const std::string& cmd): blockType(type), loopCount(0),
	command(cmd), wantBlockSign(false), wantTrueBlock(false), wantElseFlag(false), wantFalseBlock(false)
{
	if (blockType == CBT_If)
	{
		wantTrueBlock = true;
	}

	wantBlockSign = (blockType != CBT_Normal);
}

/*
//-- Debug using.
void CmdBlock::printStatus()
{
	cout <<" -- blockType: ";
	if (blockType == CBT_If) cout<<"CBT_If"<<endl;
	if (blockType == CBT_Forever) cout<<"CBT_Forever"<<endl;
	if (blockType == CBT_While) cout<<"CBT_While"<<endl;
	if (blockType == CBT_CountLoop) cout<<"CBT_CountLoop"<<endl;
	if (blockType == CBT_Normal) cout<<"CBT_Normal"<<endl;

	cout<<" -- loopCount: "<<loopCount<<endl;
	cout<<" -- command: "<<command<<endl;

	cout<<" -- wantBlockSign: "<<wantBlockSign<<endl;
	cout<<" -- wantTrueBlock: "<<wantTrueBlock<<endl;
	cout<<" -- wantElseFlag: "<<wantElseFlag<<endl;
	cout<<" -- wantFalseBlock: "<<wantFalseBlock<<endl;

	cout<<" -- commBlockChains size: "<<commBlockChains.size()<<endl;
	cout<<" -- trueBlockChains size: "<<trueBlockChains.size()<<endl;
	cout<<" -- falseBlockChains size: "<<falseBlockChains.size()<<endl;
}
*/

const std::string ScriptParser::conditionHeadString("__condition");

void ScriptParser::showScriptGrammars()
{
	cout<<"  Line start with # or // is comments."<<endl;
	cout<<endl;
	cout<<"  sleep <seconds>"<<endl;
	cout<<endl;

	cout<<"  forever loop"<<endl;
	cout<<"  {"<<endl;
	cout<<"      ... ..."<<endl;
	cout<<"  }"<<endl;
	cout<<endl;

	cout<<"  for <count> times"<<endl;
	cout<<"  {"<<endl;
	cout<<"      ... ..."<<endl;
	cout<<"  }"<<endl;
	cout<<endl;

	cout<<"  while <while_condition> ... ..."<<endl;
	cout<<endl;

	cout<<"  while <while_condition>"<<endl;
	cout<<"  {"<<endl;
	cout<<"      ... ..."<<endl;
	cout<<"  }"<<endl;
	cout<<endl;

	cout<<"  if <if_condition> ... ..."<<endl;
	cout<<endl;

	cout<<"  if <if_condition>"<<endl;
	cout<<"  {"<<endl;
	cout<<"      ... ..."<<endl;
	cout<<"  }"<<endl;
	cout<<endl;

	cout<<"  if <if_condition>"<<endl;
	cout<<"  {"<<endl;
	cout<<"      ... ..."<<endl;
	cout<<"  } else {"<<endl;
	cout<<"      ... ..."<<endl;
	cout<<"  }"<<endl;
	cout<<endl;

	cout<<"    while_condition & if_condition:"<<endl;
	cout<<"        <var_name> == <value>"<<endl;
	cout<<"        <var_name> match <var_name>"<<endl;
	cout<<"        answer is error"<<endl;
	cout<<"        answer is good"<<endl;
	cout<<endl;
	cout<<"    break"<<endl;
	cout<<endl;

	cout<<"  set var <var_name> = <value>"<<endl;
	cout<<"  store answer item <item_path> as <var_name>"<<endl;
	cout<<"  fill next quest item <item_path> with <var_name> as <string | int | double | bool>"<<endl;
	cout<<"  print value <var_name>"<<endl;
	cout<<"  print answer item <item_path>"<<endl;
}

void grammarError(const char* hint, int line)
{
	cout<<hint<<" at line "<<line<<"."<<endl;
}

void grammarError(int line)
{
	grammarError("Grammers Logic Error", line);
}

ScriptParser::ScriptParser()
{
	_cmdBlock.reset(new CmdBlock(CmdBlock::CBT_Normal));
	_cmdBlockStack.push(_cmdBlock);
}

bool ScriptParser::praseScript(const std::string& scriptFilePath)
{
	std::vector<std::string> lines;
	if (!FileSystemUtil::fetchFileContentInLines(scriptFilePath, lines, false, true))
	{
		cout<<"Read script file "<<scriptFilePath<<" failed."<<endl;
		return false;
	}

	for (size_t i = 0; i < lines.size(); i++)
	{
		std::string rawline = lines[i];
		std::string line = StringUtil::trim(rawline);

		if (line.empty())
			continue;

		if (line[0] == '#')
			continue;

		if (line[0] == '/' && line.length() >= 2 && line[1] == '/')
			continue;

		if (!parseLine(line, i + 1))
			return false;
	}

	return true;
}

bool ScriptParser::beginBlock(int lineNo)
{
	if (_cmdBlockStack.empty())
	{
		grammarError(lineNo);
		return false;
	}

	CmdBlockPtr curr = _cmdBlockStack.top();
	CmdBlockPtr block(new CmdBlock(CmdBlock::CBT_Normal));

	if (curr->blockType == CmdBlock::CBT_If)
	{
		if (curr->wantTrueBlock)
		{
			if (curr->wantBlockSign)
				curr->wantBlockSign = false;
			else
			{
				curr->trueBlockChains.push_back(block);
				_cmdBlockStack.push(block);
			}
		}
		else if (curr->wantFalseBlock)
		{
			if (curr->wantBlockSign)
				curr->wantBlockSign = false;
			else
			{
				curr->falseBlockChains.push_back(block);
				_cmdBlockStack.push(block);
			}
		}
		else
		{
			if (!finishBlock(lineNo))
				return false;

			return beginBlock(lineNo);
		}
	}
	else
	{
		if (curr->wantBlockSign)
			curr->wantBlockSign = false;
		else
		{
			curr->commBlockChains.push_back(block);
			_cmdBlockStack.push(block);
		}
	}

	return true;
}

bool ScriptParser::finishBlock(int lineNo)
{
	if (_cmdBlockStack.empty())
	{
		grammarError(lineNo);
		return false;
	}

	bool popped = false;
	CmdBlockPtr curr = _cmdBlockStack.top();

	if (curr->blockType == CmdBlock::CBT_If)
	{
		if (curr->wantTrueBlock)
		{
			if (curr->wantBlockSign && curr->trueBlockChains.empty())
			{
				grammarError(lineNo);
				return false;
			}

			curr->wantTrueBlock = false;
			curr->wantElseFlag = true;
		}
		else if (curr->wantFalseBlock)
		{
			if (curr->wantBlockSign && curr->falseBlockChains.empty())
			{
				grammarError(lineNo);
				return false;
			}

			curr->wantFalseBlock = false;
			_cmdBlockStack.pop();
			popped = true;
		}
		else
		{
			curr->wantElseFlag = false;
			_cmdBlockStack.pop();
			popped = true;
		}
	}
	else
	{
		if (curr->wantBlockSign && curr->commBlockChains.empty())
		{
			grammarError(lineNo);
			return false;
		}

		if (curr->blockType == CmdBlock::CBT_Forever && curr->commBlockChains.empty())
		{
			grammarError("Empty cmds for forever loop.", lineNo);
			return false;
		}

		_cmdBlockStack.pop();
		popped = true;
	}

	if (popped)
		return checkFinishBlock(lineNo);

	return true;
}

bool ScriptParser::checkFinishBlock(int lineNo)
{
	if (_cmdBlockStack.empty())
		return true;

	CmdBlockPtr curr = _cmdBlockStack.top();

	if (curr->blockType == CmdBlock::CBT_If)
	{
		if (curr->wantTrueBlock)
		{
			if (curr->wantBlockSign && curr->trueBlockChains.size())
				return finishBlock(lineNo);
		}
		else if (curr->wantFalseBlock)
		{
			if (curr->wantBlockSign && curr->falseBlockChains.size())
				return finishBlock(lineNo);
		}
	}
	else
	{
		if (curr->wantBlockSign && curr->commBlockChains.size())
			return finishBlock(lineNo);
	}

	return true;
}

bool ScriptParser::addConditionCmdBlock(CmdBlockPtr block, int lineNo)
{
	CmdBlockPtr curr = _cmdBlockStack.top();

	if (curr->blockType == CmdBlock::CBT_If)
	{
		if (curr->wantTrueBlock)
		{
			curr->trueBlockChains.push_back(block);
			_cmdBlockStack.push(block);

			if (curr->wantBlockSign)
			{
				curr->wantBlockSign = false;
				curr->wantTrueBlock = false;
				curr->wantElseFlag = true;
			}
		}
		else if (curr->wantFalseBlock)
		{
			curr->falseBlockChains.push_back(block);
			_cmdBlockStack.push(block);

			if (curr->wantBlockSign)
			{
				curr->wantBlockSign = false;
				curr->wantBlockSign = false;
			}
		}
		else
		{
			if (!finishBlock(lineNo))
				return false;

			return addConditionCmdBlock(block, lineNo);
		}
	}
	else
	{
		curr->commBlockChains.push_back(block);
		_cmdBlockStack.push(block);

		if (curr->wantBlockSign)
			curr->wantBlockSign = false;
	}

	return true;
}

bool ScriptParser::parseForever(std::vector<std::string>& cmd, int lineNo)
{
	if (cmd.size() < 2 || _cmdBlockStack.empty())
	{
		grammarError(lineNo);
		return false;
	}

	CmdBlockPtr block(new CmdBlock(CmdBlock::CBT_Forever));

	//-- check
	if (cmd[1] == "loop")
	{
		if (!addConditionCmdBlock(block, lineNo))
			return false;

		cmd.erase(cmd.begin());
		cmd.erase(cmd.begin());
	}
	else if (strncmp(cmd[1].c_str(), "loop", 4) == 0)
	{
		if (!addConditionCmdBlock(block, lineNo))
			return false;

		cmd.erase(cmd.begin());
		cmd[0].erase(0, 4);

		if (cmd[0][0] != '{')
		{
			grammarError(lineNo);
			return false;
		}
	}
	else
	{
		grammarError(lineNo);
		return false;
	}

	if (cmd.empty())
		return true;

	std::string restCmd = StringUtil::join(cmd, " ");
	return parseLine(restCmd, lineNo);
}

bool ScriptParser::parseCountLoop(std::vector<std::string>& cmd, int lineNo)
{
	if (cmd.size() < 3 || _cmdBlockStack.empty())
	{
		grammarError(lineNo);
		return false;
	}

	CmdBlockPtr block(new CmdBlock(CmdBlock::CBT_CountLoop));

	bool error;
	block->loopCount = (int)convertInt(cmd[1], error);
	if (error)
	{
		grammarError("Convert string to integer error", lineNo);
		return false;
	}

	//-- check
	if (cmd[2] == "times")
	{
		if (!addConditionCmdBlock(block, lineNo))
			return false;

		cmd.erase(cmd.begin());
		cmd.erase(cmd.begin());
		cmd.erase(cmd.begin());
	}
	else if (strncmp(cmd[1].c_str(), "times", 5) == 0)
	{
		if (!addConditionCmdBlock(block, lineNo))
			return false;

		cmd.erase(cmd.begin());
		cmd.erase(cmd.begin());
		cmd[0].erase(0, 5);

		if (cmd[0][0] != '{')
		{
			grammarError(lineNo);
			return false;
		}
	}
	else
	{
		grammarError(lineNo);
		return false;
	}

	if (cmd.empty())
		return true;

	std::string restCmd = StringUtil::join(cmd, " ");
	return parseLine(restCmd, lineNo);
}

bool ScriptParser::parseWhileLoop(std::vector<std::string>& cmd, int lineNo)
{
	if (cmd.size() < 4 || _cmdBlockStack.empty())
	{
		grammarError(lineNo);
		return false;
	}

	CmdBlockPtr block(new CmdBlock(CmdBlock::CBT_While));
	block->command.append(conditionHeadString).append(" ");
	block->command.append(cmd[1]).append(" ").append(cmd[2]).append(" ");

	bool error = false;
	if (cmd[2] == "==" || cmd[2] == "match")
	{
		//if (cmd[3][cmd[3].length() - 1] != '{')
		//	error = true;
		//else
		{
			block->command.append(cmd[3]);

			cmd.erase(cmd.begin());
			cmd.erase(cmd.begin());
			cmd.erase(cmd.begin());
			cmd.erase(cmd.begin());
		}
	}
	else if (cmd[2] == "is")
	{
		cmd.erase(cmd.begin());
		cmd.erase(cmd.begin());
		cmd.erase(cmd.begin());

		if (cmd[0] == "error" || cmd[0] == "good")
		{
			block->command.append(cmd[0]);
			cmd.erase(cmd.begin());
		}
		else if (strncmp(cmd[0].c_str(), "error", 5) == 0)
		{
			block->command.append("error");
			cmd[0].erase(0, 5);

			if (cmd[0][0] != '{')
				error = true;
		}
		else if (strncmp(cmd[0].c_str(), "good", 4) == 0)
		{
			block->command.append("good");
			cmd[0].erase(0, 4);

			if (cmd[0][0] != '{')
				error = true;
		}
		else
			error = true;
	}
	else
		error = true;

	if (error)
	{
		grammarError("While condition error", lineNo);
		return false;
	}

	if (!addConditionCmdBlock(block, lineNo))
		return false;

	if (cmd.empty())
		return true;

	std::string restCmd = StringUtil::join(cmd, " ");
	return parseLine(restCmd, lineNo);
}

bool ScriptParser::parseIf(std::vector<std::string>& cmd, int lineNo)
{
	if (cmd.size() < 4 || _cmdBlockStack.empty())
	{
		grammarError(lineNo);
		return false;
	}

	CmdBlockPtr block(new CmdBlock(CmdBlock::CBT_If));
	block->command.append(conditionHeadString).append(" ");
	block->command.append(cmd[1]).append(" ").append(cmd[2]).append(" ");

	bool error = false;
	if (cmd[2] == "==" || cmd[2] == "match")
	{
		//if (cmd[3][cmd[3].length() - 1] != '{')
		//	error = true;
		//else
		{
			block->command.append(cmd[3]);

			cmd.erase(cmd.begin());
			cmd.erase(cmd.begin());
			cmd.erase(cmd.begin());
			cmd.erase(cmd.begin());
		}
	}
	else if (cmd[2] == "is")
	{
		cmd.erase(cmd.begin());
		cmd.erase(cmd.begin());
		cmd.erase(cmd.begin());

		if (cmd[0] == "error" || cmd[0] == "good")
		{
			block->command.append(cmd[0]);
			cmd.erase(cmd.begin());
		}
		else if (strncmp(cmd[0].c_str(), "error", 5) == 0)
		{
			block->command.append("error");
			cmd[0].erase(0, 5);

			if (cmd[0][0] != '{')
				error = true;
		}
		else if (strncmp(cmd[0].c_str(), "good", 4) == 0)
		{
			block->command.append("good");
			cmd[0].erase(0, 4);

			if (cmd[0][0] != '{')
				error = true;
		}
		else
			error = true;
	}
	else
		error = true;

	if (error)
	{
		grammarError("If condition error", lineNo);
		return false;
	}

	if (!addConditionCmdBlock(block, lineNo))
		return false;

	if (cmd.empty())
		return true;

	std::string restCmd = StringUtil::join(cmd, " ");
	return parseLine(restCmd, lineNo);
}

bool ScriptParser::parseElse(std::vector<std::string>& cmd, int lineNo)
{
	if (_cmdBlockStack.empty())
	{
		grammarError(lineNo);
		return false;
	}

	if (cmd[0] == "else")
		cmd.erase(cmd.begin());
	else
	{
		cmd[0].erase(0, 4);

		if (cmd[0][0] != '{')
		{
			grammarError(lineNo);
			return false;
		}
	}

	CmdBlockPtr curr = _cmdBlockStack.top();
	if (curr->blockType == CmdBlock::CBT_If && curr->wantElseFlag)
	{
		curr->wantElseFlag = false;
		curr->wantBlockSign = true;
		curr->wantFalseBlock = true;
	}
	else
	{
		grammarError(lineNo);
		return false;
	}

	if (cmd.empty())
		return true;

	std::string restCmd = StringUtil::join(cmd, " ");
	return parseLine(restCmd, lineNo);
}

bool ScriptParser::fillCmd(const std::string& cmd, int lineNo)
{
	if (_cmdBlockStack.empty())
	{
		grammarError(lineNo);
		return false;
	}

	CmdBlockPtr curr = _cmdBlockStack.top();
	CmdBlockPtr block(new CmdBlock(CmdBlock::CBT_Normal, cmd));

	if (curr->blockType == CmdBlock::CBT_If)
	{
		if (curr->wantTrueBlock)
		{
			curr->trueBlockChains.push_back(block);
			return checkFinishBlock(lineNo);
		}
		else if (curr->wantFalseBlock)
		{
			curr->falseBlockChains.push_back(block);
			return checkFinishBlock(lineNo);
		}
		else
		{
			curr->wantElseFlag = false;
			_cmdBlockStack.pop();

			return checkFinishBlock(lineNo);
		}
	}
	else
	{
		curr->commBlockChains.push_back(block);
		return checkFinishBlock(lineNo);
	}
}

bool ScriptParser::parseLine(std::string& cmd, int lineNo)
{
	bool blockCheck = true;
	while (blockCheck && !cmd.empty())
	{
		char headChar = cmd[0];

		if (headChar == '{')
		{
			if (!beginBlock(lineNo))
				return false;

			cmd.erase(0, 1);
		}
		else if (headChar == '}')
		{
			if (!finishBlock(lineNo))
				return false;

			cmd.erase(0, 1);
		}
		else if (isspace(headChar))
			cmd.erase(0, 1);
		else
			blockCheck = false;
	}

	if (cmd.empty())
		return true;

	std::vector<std::string> cmdSections = DisassembleCommand::disassemble(cmd);
	if (cmdSections[0] == "forever")
		return parseForever(cmdSections, lineNo);

	if (cmdSections[0] == "for")
		return parseCountLoop(cmdSections, lineNo);

	if (cmdSections[0] == "while")
		return parseWhileLoop(cmdSections, lineNo);

	if (cmdSections[0] == "if")
		return parseIf(cmdSections, lineNo);

	if (strncmp(cmdSections[0].c_str(), "else", 4) == 0)
		return parseElse(cmdSections, lineNo);

	return fillCmd(cmd, lineNo);
}

CmdBlockPtr ScriptParser::parse(const std::string& scriptFilePath)
{
	ScriptParser parser;
	if (parser.praseScript(scriptFilePath))
		return parser._cmdBlock;
	else
		return nullptr;
}

void ScriptParser::showScriptStruct(CmdBlockPtr cmd, int depth)
{
	std::string indent(2 + depth * 4, ' ');

	if (cmd->blockType == CmdBlock::CBT_If)
	{
		cout<<indent<<"if "<<cmd->command<<endl;
		cout<<indent<<"{"<<endl;
		
		if (cmd->trueBlockChains.size())
		{
			for (size_t i = 0; i < cmd->trueBlockChains.size(); i++)
				showScriptStruct(cmd->trueBlockChains[i], depth + 1);
		}

		cout<<indent<<"}"<<endl;

		if (cmd->falseBlockChains.size())
		{
			cout<<indent<<"else"<<endl;
			cout<<indent<<"{"<<endl;

			for (size_t i = 0; i < cmd->falseBlockChains.size(); i++)
				showScriptStruct(cmd->falseBlockChains[i], depth + 1);

			cout<<indent<<"}"<<endl;
		}
	}
	else if (cmd->blockType == CmdBlock::CBT_Forever)
	{
		cout<<indent<<"forever loop"<<endl;
		cout<<indent<<"{"<<endl;
		
		if (cmd->commBlockChains.size())
		{
			for (size_t i = 0; i < cmd->commBlockChains.size(); i++)
				showScriptStruct(cmd->commBlockChains[i], depth + 1);
		}

		cout<<indent<<"}"<<endl;
	}
	else if (cmd->blockType == CmdBlock::CBT_While)
	{
		cout<<indent<<"while "<<cmd->command<<endl;
		cout<<indent<<"{"<<endl;
		
		if (cmd->commBlockChains.size())
		{
			for (size_t i = 0; i < cmd->commBlockChains.size(); i++)
				showScriptStruct(cmd->commBlockChains[i], depth + 1);
		}

		cout<<indent<<"}"<<endl;
	}
	else if (cmd->blockType == CmdBlock::CBT_CountLoop)
	{
		cout<<indent<<"for "<<cmd->loopCount<<" times"<<endl;
		cout<<indent<<"{"<<endl;

		if (cmd->commBlockChains.size())
		{
			for (size_t i = 0; i < cmd->commBlockChains.size(); i++)
				showScriptStruct(cmd->commBlockChains[i], depth + 1);
		}

		cout<<indent<<"}"<<endl;
	}
	else //-- if (cmd->blockType == CmdBlock::CBT_Normal)
	{
		if (cmd->command.size())
			cout<<indent<<cmd->command<<endl;

		if (cmd->commBlockChains.size())
		{
			for (size_t i = 0; i < cmd->commBlockChains.size(); i++)
				showScriptStruct(cmd->commBlockChains[i], depth + 1);
		}
	}
}

void ScriptParser::showScriptStruct(const std::string& scriptFilePath)
{
	ScriptParser parser;
	if (!parser.praseScript(scriptFilePath))
		return;

	CmdBlockPtr cmds = parser._cmdBlock;
	showScriptStruct(cmds, 0);
}