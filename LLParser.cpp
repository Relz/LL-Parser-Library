#include "LLParser.h"
#include <stack>
#include "LLTableBuilderLibrary/Table/TableRow/TableRow.h"

LLParser::LLParser(std::string const & inputFileName)
	: m_llTableBuilder(inputFileName)
{
}

bool LLParser::IsValid(
		std::vector<std::string> const & inputWords,
		size_t & failIndex,
		std::unordered_set<std::string> & expectedWords
) const
{
	Table const & table = m_llTableBuilder.GetTable();
	std::stack<unsigned int> stack;
	size_t inputWordIndex = 0;
	unsigned int currentRowId = 1;
	while (inputWordIndex <= inputWords.size())
	{
		std::string const & currentWord =
				inputWordIndex < inputWords.size() ? inputWords.at(inputWordIndex) : END_OF_LINE_STRING;
		TableRow * currentRow = table.GetRow(currentRowId);
		if (currentRow == nullptr)
		{
			return false;
		}
		if (currentRow->referencingSet.find(currentWord) != currentRow->referencingSet.end())
		{
			if (currentRow->isEnd && stack.empty())
			{
				return true;
			}
			if (currentRow->doShift)
			{
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
	return false;
}

std::string const LLParser::END_OF_LINE_STRING = "#";
