#include "LLParser.h"
#include "LLTableBuilderLibrary/Table/TableRow/TableRow.h"
#include "LLTableBuilderLibrary/TokenLibrary/TokenInformation/TokenInformation.h"
#include "LLTableBuilderLibrary/TokenLibrary/TokenExtensions/TokenExtensions.h"
#include "LexerLibrary/Lexer.h"
#include "LexerLibrary/TokenLibrary/TokenInformation/TokenInformation.h"
#include <string>
#include <functional>
#include <regex>

LLParser::LLParser(std::string const & ruleFileName)
	: m_llTableBuilder(ruleFileName)
{
}

bool LLParser::IsValid(
	std::string const & inputFileName,
	std::vector<TokenInformation> & tokenInformations,
	size_t & failIndex,
	std::unordered_set<Token> & expectedTokens
) const
{
	bool result;
	Lexer lexer(inputFileName);
	Table const & table = m_llTableBuilder.GetTable();
	std::stack<unsigned int> stack;
	size_t inputWordIndex = 0;
	unsigned int currentRowId = 1;
	TokenInformation tokenInformation;
	//AstNode * astFather = new AstNode();
	std::stack<AstNode *> astStack;
	bool newToken = true;
	if (!lexer.GetNextTokenInformation(tokenInformation))
	{
		return false;
	}
	tokenInformations.emplace_back(tokenInformation);
	while (true)
	{
		Token currentToken = tokenInformation.GetToken();
		TableRow * currentRow = table.GetRow(currentRowId);
		if (currentRow == nullptr)
		{
			result = false;
			break;
		}
		if (newToken)
		{
			newToken = false;
		}
		ResolveActionName(currentRow->actionName);
		if (currentRow->referencingSet.find(currentToken) != currentRow->referencingSet.end() || !currentRow->actionName.empty())
		{
			if (currentRow->isEnd && stack.empty())
			{
				astStack.push(new AstNode());
				astStack.top()->name = TokenExtensions::ToString(currentToken);
				astStack.top()->stringValue = tokenInformation.GetTokenStreamString().string;
				ResolveAstActionName(currentRow->actionName, astStack);
				result = true;
				break;
			}
			if (currentRow->doShift)
			{
				astStack.push(new AstNode());
				astStack.top()->name = TokenExtensions::ToString(currentToken);
				astStack.top()->stringValue = tokenInformation.GetTokenStreamString().string;
				if (!lexer.GetNextTokenInformation(tokenInformation))
				{
					result = false;
					break;
				}
				tokenInformations.emplace_back(tokenInformation);
				++inputWordIndex;
				newToken = true;
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
					result = false;
					break;
				}
				currentRowId = stack.top();
				stack.pop();
				ResolveAstActionName(currentRow->actionName, astStack);
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

AstNode * LLParser::CreateAstNode(
	std::string const & ruleName, unsigned int tokenCount, std::stack<AstNode *> & astStack
) const
{
	AstNode * astNode = new AstNode();
	astNode->name = ruleName;
	for (unsigned int i = 0; i < tokenCount; ++i)
	{
		astNode->children.insert(astNode->children.begin(), astStack.top());
		astStack.top()->father = astNode;
		if (astStack.empty())
		{
			throw std::runtime_error(
				"AST Node \"" + astNode->name + "\" requires " + std::to_string(tokenCount)
				+ ", but " + std::to_string(astNode->children.size()) + " found"
			);
		}
		astStack.pop();
	}

	return astNode;
}

bool LLParser::ParseCreateAstNodeAction(
	std::string const & actionName, std::string & ruleName, unsigned int & tokenCount
)
{
	static std::regex regEx("CreateAstNode_(.+)_Using_([0-9]+)");
	std::smatch match;
	if (std::regex_search(actionName, match, regEx))
	{
		ruleName = match[1];
		tokenCount = stoul(match[2]);
		return true;
	}
	return false;
}

bool LLParser::TryToCreateAstNode(
	std::string const & actionName, std::stack<AstNode *> & astStack
) const
{
	std::string ruleName;
	unsigned int tokenCount;
	if (LLParser::ParseCreateAstNodeAction(actionName, ruleName, tokenCount))
	{
		AstNode * astNode = CreateAstNode(ruleName, tokenCount, astStack);
		astStack.push(astNode);
		return true;
	}
	return false;
}

void LLParser::ResolveActionName(std::string const & actionName) const
{
	if (actionName.empty())
	{
		return;
	}
	if (ACTION_NAME_TO_ACTION_MAP.find(actionName) == ACTION_NAME_TO_ACTION_MAP.end())
	{
		std::string tmp0;
		unsigned int tmp1;
		if (!ParseCreateAstNodeAction(actionName, tmp0, tmp1))
		{
			std::cout << "Warning: unhandled action name: " << actionName << "\n";
		}
	}
	else
	{
		ACTION_NAME_TO_ACTION_MAP.at(actionName)();
	}
}

void LLParser::ResolveAstActionName(std::string const & actionName, std::stack<AstNode *> & astStack) const
{
	if (actionName.empty())
	{
		return;
	}
	if (ACTION_NAME_TO_ACTION_MAP.find(actionName) == ACTION_NAME_TO_ACTION_MAP.end())
	{
		if (!TryToCreateAstNode(actionName, astStack))
		{
			std::cout << "Warning: unhandled action name: " << actionName << "\n";
		}
	}
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
