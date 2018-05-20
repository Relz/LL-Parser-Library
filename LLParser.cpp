#include "LLParser.h"
#include <stack>
#include "LLTableBuilderLibrary/Table/TableRow/TableRow.h"
#include "LexerLibrary/Lexer.h"
#include "LexerLibrary/TokenLibrary/TokenInformation/TokenInformation.h"

LLParser::LLParser(std::string const & ruleFileName)
	: m_llTableBuilder(ruleFileName)
{
}

bool LLParser::IsValid(
	std::string const & inputFileName,
	size_t & failIndex,
	std::unordered_set<std::string> & expectedWords) const
{
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
	while (true)
	{
		std::string const & currentToken = TokenExtensions::ToString(tokenInformation.GetToken());
		TableRow * currentRow = table.GetRow(currentRowId);
		if (currentRow == nullptr)
		{
			return false;
		}
		if (currentRow->referencingSet.find(currentToken) != currentRow->referencingSet.end())
		{
			if (currentRow->isEnd && stack.empty())
			{
				return true;
			}
			if (currentRow->doShift)
			{
				if (!lexer.GetNextTokenInformation(tokenInformation))
				{
					return false;
				}
				++inputWordIndex;
			}
			else if (currentRow->pushToStack != 0)
			{
				stack.push(currentRow->pushToStack);
			}
			if (currentRow->nextId != 0)
			{
				currentRowId = currentRow->nextId;
			}
			else
			{
				if (stack.empty())
				{
					return false;
				}
				currentRowId = stack.top();
				stack.pop();
			}
		}
		else if (currentRow->isError)
		{
			failIndex = inputWordIndex;
			expectedWords.insert(
				std::make_move_iterator(currentRow->referencingSet.begin()),
				std::make_move_iterator(currentRow->referencingSet.end()));
			--currentRowId;
			currentRow = table.GetRow(currentRowId);
			while (currentRow != nullptr && !currentRow->isError)
			{
				expectedWords.insert(
					std::make_move_iterator(currentRow->referencingSet.begin()),
					std::make_move_iterator(currentRow->referencingSet.end()));
				--currentRowId;
				currentRow = table.GetRow(currentRowId);
			}
			return false;
		}
		else
		{
			++currentRowId;
		}
	}
}
