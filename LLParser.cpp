#include "LLParser.h"
#include "LLTableBuilderLibrary/Table/TableRow/TableRow.h"
#include "LLTableBuilderLibrary/TokenLibrary/TokenInformation/TokenInformation.h"
#include "LLTableBuilderLibrary/TokenLibrary/TokenExtensions/TokenExtensions.h"
#include "LexerLibrary/Lexer.h"
#include "LexerLibrary/TokenLibrary/TokenInformation/TokenInformation.h"
#include <string>
#include <stack>
#include <functional>

LLParser::LLParser(std::string const & ruleFileName)
	: m_llTableBuilder(ruleFileName)
{
}

bool LLParser::IsValid(
	std::string const & inputFileName,
	std::vector<TokenInformation> & tokenInformations,
	size_t & failIndex,
	std::unordered_set<Token> & expectedTokens) const
{
	bool result;
	Lexer lexer(inputFileName);
	Table const & table = m_llTableBuilder.GetTable();
	std::stack<unsigned int> stack;
	size_t inputWordIndex = 0;
	unsigned int currentRowId = 1;
	TokenInformation tokenInformation;
	if (!lexer.GetNextTokenInformation(tokenInformation))
	{
		return false;
	}
	tokenInformations.emplace_back(std::move(tokenInformation));
	while (true)
	{
		Token currentToken = tokenInformation.GetToken();
		TableRow * currentRow = table.GetRow(currentRowId);
		if (currentRow == nullptr)
		{
			result = false;
			break;
		}
		if (currentRow->referencingSet.find(currentToken) != currentRow->referencingSet.end())
		{
			if (currentRow->isEnd && stack.empty())
			{
				result = true;
				break;
			}
			if (currentRow->doShift)
			{
				if (!lexer.GetNextTokenInformation(tokenInformation))
				{
					result = false;
					break;
				}
				tokenInformations.emplace_back(std::move(tokenInformation));
				++inputWordIndex;
			}
			else if (currentRow->pushToStack != 0)
			{
				stack.push(currentRow->pushToStack);
			}
			if (currentRow->nextId != 0)
			{
				currentRowId = currentRow->nextId;
				if (!currentRow->actionName.empty())
				{
					if (ACTION_NAME_TO_ACTION_MAP.find(currentRow->actionName) != ACTION_NAME_TO_ACTION_MAP.end())
					{
						ACTION_NAME_TO_ACTION_MAP.at(currentRow->actionName)();
					}
				}
			}
			else
			{
				if (stack.empty())
				{
					result = false;
					break;
				}
				currentRowId = stack.top();
				stack.pop();
				if (!currentRow->actionName.empty())
				{
					if (ACTION_NAME_TO_ACTION_MAP.find(currentRow->actionName) != ACTION_NAME_TO_ACTION_MAP.end())
					{
						ACTION_NAME_TO_ACTION_MAP.at(currentRow->actionName)();
					}
				}
			}
		}
		else if (currentRow->isError)
		{
			failIndex = inputWordIndex;
			expectedTokens.insert(
				std::make_move_iterator(currentRow->referencingSet.begin()),
				std::make_move_iterator(currentRow->referencingSet.end()));
			--currentRowId;
			currentRow = table.GetRow(currentRowId);
			while (currentRow != nullptr && !currentRow->isError)
			{
				expectedTokens.insert(
					std::make_move_iterator(currentRow->referencingSet.begin()),
					std::make_move_iterator(currentRow->referencingSet.end()));
				--currentRowId;
				currentRow = table.GetRow(currentRowId);
			}
			result = false;
			break;
		}
		else
		{
			++currentRowId;
		}
	}
	while (lexer.GetNextTokenInformation(tokenInformation))
	{
		tokenInformations.emplace_back(std::move(tokenInformation));
	}
	return result;
}

void LLParser::ProgramAction()
{
	std::cout << "ProgramAction!" << "\n";
}

void LLParser::VariableDeclarationAction()
{
	std::cout << "VariableDeclarationAction!" << "\n";
}

void LLParser::AssignmentAction()
{
	std::cout << "AssignmentAction!" << "\n";
}
