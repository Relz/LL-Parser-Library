#include "LLParser.h"
#include "LLTableBuilderLibrary/Table/TableRow/TableRow.h"
#include "LLTableBuilderLibrary/TokenLibrary/TokenInformation/TokenInformation.h"
#include "LLTableBuilderLibrary/TokenLibrary/TokenExtensions/TokenExtensions.h"
#include "LexerLibrary/Lexer.h"
#include "LexerLibrary/TokenLibrary/TokenInformation/TokenInformation.h"
#include "Calculator/Calculator.h"
#include <string>
#include <functional>
#include <regex>
#include <unordered_set>

LLParser::LLParser(std::string const & ruleFileName)
	: m_llTableBuilder(ruleFileName)
{
}

bool LLParser::IsValid(
	std::string const & inputFileName,
	std::vector<TokenInformation> & tokenInformations,
	size_t & failIndex,
	std::unordered_set<Token> & expectedTokens
)
{
	bool result;
	Lexer lexer(inputFileName);
	Table const & table = m_llTableBuilder.GetTable();
	std::stack<unsigned int> stack;
	size_t inputWordIndex = 0;
	unsigned int currentRowId = 1;
	TokenInformation tokenInformation;
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
		if (!ResolveActionName(currentRow->actionName))
		{
			failIndex = inputWordIndex;
			result = false;
			break;
		}
		if (currentToken == Token::LINE_COMMENT || currentToken == Token::BLOCK_COMMENT)
		{
			if (!lexer.GetNextTokenInformation(tokenInformation))
			{
				result = false;
				break;
			}
			tokenInformations.emplace_back(tokenInformation);
			++inputWordIndex;
			continue;
		}
		if (currentRow->referencingSet.find(currentToken) != currentRow->referencingSet.end() || !currentRow->actionName.empty())
		{
			if (currentRow->isEnd && stack.empty())
			{
				m_ast.emplace_back(new AstNode());
				m_ast.back()->name = TokenExtensions::ToString(currentToken);
				m_ast.back()->type = m_ast.back()->name;
				m_ast.back()->computedType = m_ast.back()->name;
				m_ast.back()->stringValue = tokenInformation.GetTokenStreamString().string;
				if (!ResolveAstActionName(currentRow->actionName))
				{
					failIndex = inputWordIndex;
					result = false;
					break;
				}
				result = true;
				break;
			}
			if (currentRow->doShift)
			{
				m_ast.emplace_back(new AstNode());
				m_ast.back()->name = TokenExtensions::ToString(currentToken);
				m_ast.back()->type = m_ast.back()->name;
				m_ast.back()->computedType = m_ast.back()->name;
				m_ast.back()->stringValue = tokenInformation.GetTokenStreamString().string;
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
				if (!ResolveAstActionName(currentRow->actionName))
				{
					failIndex = inputWordIndex;
					result = false;
					break;
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

AstNode * LLParser::CreateAstNode(
	std::string const & ruleName, unsigned int tokenCount
)
{
	AstNode * astNode = new AstNode();
	astNode->name = ruleName;
	for (unsigned int i = 0; i < tokenCount; ++i)
	{
		astNode->children.insert(astNode->children.begin(), m_ast.back());
		m_ast.back()->father = astNode;
		if (m_ast.empty())
		{
			throw std::runtime_error(
				"AST Node \"" + astNode->name + "\" requires " + std::to_string(tokenCount)
				+ ", but " + std::to_string(astNode->children.size()) + " found"
			);
		}
		m_ast.pop_back();
	}

	return astNode;
}

bool LLParser::ParseCreateAstNodeAction(
	std::string const & actionName, std::string & ruleName, unsigned int & tokenCount
)
{
	static std::regex regEx("Create AST node (.+) using ([0-9]+)");
	std::smatch match;
	if (std::regex_search(actionName, match, regEx))
	{
		ruleName = match[1];
		tokenCount = stoul(match[2]);
		return true;
	}
	return false;
}

bool LLParser::TryToCreateAstNode(std::string const & actionName)
{
	std::string ruleName;
	unsigned int tokenCount;
	if (LLParser::ParseCreateAstNodeAction(actionName, ruleName, tokenCount))
	{
		AstNode * astNode = CreateAstNode(ruleName, tokenCount);
		m_ast.emplace_back(astNode);

		std::vector<AstNode*> synthesisChildren;
		std::copy_if(
			astNode->children.begin(),
			astNode->children.end(),
			std::back_inserter(synthesisChildren),
			[](AstNode * child)
			{
				return !child->stringValue.empty() || !child->children.empty();
			}
		);
		if (synthesisChildren.empty())
		{
			return true;
		}
		std::string synthesisActionName = "Synthesis";
		if (synthesisChildren.size() > 1)
		{
			for (AstNode * child : synthesisChildren)
			{
				synthesisActionName += " " + child->name;
			}
		}
		return ResolveActionName(synthesisActionName);
	}
	return false;
}

bool LLParser::ResolveActionName(std::string const & actionName) const
{
	if (actionName.empty() || IGNORED_ACTION_NAME.find(actionName) != IGNORED_ACTION_NAME.end())
	{
		return true;
	}
	if (ACTION_NAME_TO_ACTION_MAP.find(actionName) == ACTION_NAME_TO_ACTION_MAP.end())
	{
		std::string tmp0;
		unsigned int tmp1;
		if (!ParseCreateAstNodeAction(actionName, tmp0, tmp1))
		{
			PrintWarningMessage("Unhandled action name: \"" + actionName + "\"" + "\n");
		}
	}
	else
	{
		return ACTION_NAME_TO_ACTION_MAP.at(actionName)();
	}
	return true;
}

bool LLParser::ResolveAstActionName(std::string const & actionName)
{
	if (actionName.empty())
	{
		return true;
	}
	if (ACTION_NAME_TO_ACTION_MAP.find(actionName) == ACTION_NAME_TO_ACTION_MAP.end())
	{
		return TryToCreateAstNode(actionName);
	}
	return true;
}

bool LLParser::CreateScopeAction()
{
	m_scopes.emplace_back(std::unordered_map<std::string, unsigned int>());

	return true;
}

bool LLParser::DestroyScopeAction()
{
	for (std::pair<std::string, unsigned int> scopeElement : m_scopes.back())
	{
		unsigned int & symbolTableRowIndex = scopeElement.second;
		m_symbolTable.RemoveRow(symbolTableRowIndex);
	}
	m_scopes.pop_back();

	return true;
}

void LLParser::computeDimensions(std::vector<unsigned int> & dimensions)
{

}

bool LLParser::AddVariableToScope()
{
	std::vector<AstNode*> & extendedType = m_ast[m_ast.size() - 3]->children;
	std::string & variableType = extendedType[0]->stringValue;
	std::string & variableName = m_ast[m_ast.size() - 3]->children[1]->stringValue;
	std::vector<unsigned int> dimensions;
	computeDimensions(dimensions);
	m_scopes.back()[variableName] = m_symbolTable.CreateRow(variableType, variableName, dimensions);

	return true;
}

bool LLParser::CheckIdentifierForAlreadyExisting()
{
	std::string & identifierNameToCheck = m_ast[m_ast.size() - 1]->stringValue;
	for (std::unordered_map<std::string, unsigned int> & scope : m_scopes)
	{
		if (scope.find(identifierNameToCheck) != scope.end())
		{
			PrintErrorMessage("Redeclaring identifier \"" + identifierNameToCheck + "\"");

			return false;
		}
	}
	return true;
}

bool LLParser::CheckIdentifierForExisting()
{
	std::string & identifierNameToCheck = m_ast[m_ast.size() - 1]->stringValue;
	for (std::unordered_map<std::string, unsigned int> & scope : m_scopes)
	{
		if (scope.find(identifierNameToCheck) != scope.end())
		{
			return true;
		}
	}
	PrintErrorMessage("Undeclared identifier \"" + identifierNameToCheck + "\"");

	return false;
}

bool LLParser::Synthesis()
{
	m_ast[m_ast.size() - 1] = m_ast[m_ast.size() - 1]->children.front();

	return true;
}

bool LLParser::SynthesisPlus()
{
	bool identifiersExists = false;
	std::string lhsType = m_ast[m_ast.size() - 2]->type;
	std::string & lhs = m_ast[m_ast.size() - 2]->stringValue;
	if (lhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (m_ast[m_ast.size() - 2]->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByName(lhs, symbolTableRow);
			lhsType = symbolTableRow.type;
		}
		else
		{
			lhsType = m_ast[m_ast.size() - 2]->computedType;
		}
	}
	std::string rhsType = m_ast[m_ast.size() - 1]->children[1]->type;
	std::string & rhs = m_ast[m_ast.size() - 1]->children[1]->stringValue;
	if (rhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (m_ast[m_ast.size() - 1]->children[1]->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByName(rhs, symbolTableRow);
			rhsType = symbolTableRow.type;
		}
		else
		{
			rhsType = m_ast[m_ast.size() - 1]->children[1]->computedType;
		}
	}
	std::string & resultType = lhsType;
	bool areTypesCompatible = true;
	if (lhsType != rhsType)
	{
		if (EXTRA_COMPATIBLE_TYPES.find(lhsType) == EXTRA_COMPATIBLE_TYPES.end())
		{
			if (EXTRA_COMPATIBLE_TYPES.find(rhsType) == EXTRA_COMPATIBLE_TYPES.end())
			{
				areTypesCompatible = false;
			}
			else
			{
				std::unordered_set<std::string> & rhsExtraCompatibleTypes = EXTRA_COMPATIBLE_TYPES.at(rhsType);
				if (rhsExtraCompatibleTypes.find(lhsType) == rhsExtraCompatibleTypes.end())
				{
					areTypesCompatible = false;
				}
				else
				{
					resultType = rhsType;
				}
			}
		}
		else
		{
			std::unordered_set<std::string> & lhsExtraCompatibleTypes = EXTRA_COMPATIBLE_TYPES.at(lhsType);
			if (lhsExtraCompatibleTypes.find(rhsType) == lhsExtraCompatibleTypes.end())
			{
				areTypesCompatible = false;
			}
		}
	}
	if (!areTypesCompatible)
	{
		PrintErrorMessage(
			"Cannot add \"" + lhs + "\"" + "(" + "\"" + lhsType + "\"" + " type" + ")"
			+ " with " + "\"" + rhs + "\"" + "(" + "\"" + rhsType + "\"" + " type" + ")" + "\n");

		return false;
	}
	if (identifiersExists)
	{
		m_ast[m_ast.size() - 2]->type = TokenConstant::Name::IDENTIFIER;
		m_ast[m_ast.size() - 2]->computedType = resultType;
	}
	else
	{
		std::string operationResult;
		std::string errorMessage;
		if (!Calculator::Add(lhs, rhs, resultType, operationResult, errorMessage))
		{
			PrintErrorMessage(errorMessage);

			return false;
		}
		m_ast[m_ast.size() - 2]->type = resultType;
		m_ast[m_ast.size() - 2]->computedType = resultType;
		m_ast[m_ast.size() - 2]->stringValue = operationResult;
		m_ast[m_ast.size() - 2]->children.clear();
		m_ast[m_ast.size() - 1]->children.clear();
	}

	return true;
}

bool LLParser::SynthesisMinus()
{
	bool identifiersExists = false;
	std::string lhsType = m_ast[m_ast.size() - 2]->type;
	std::string & lhs = m_ast[m_ast.size() - 2]->stringValue;
	if (lhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (m_ast[m_ast.size() - 2]->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByName(lhs, symbolTableRow);
			lhsType = symbolTableRow.type;
		}
		else
		{
			lhsType = m_ast[m_ast.size() - 2]->computedType;
		}
	}
	std::string rhsType = m_ast[m_ast.size() - 1]->children[1]->type;
	std::string & rhs = m_ast[m_ast.size() - 1]->children[1]->stringValue;
	if (rhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (m_ast[m_ast.size() - 1]->children[1]->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByName(rhs, symbolTableRow);
			rhsType = symbolTableRow.type;
		}
		else
		{
			rhsType = m_ast[m_ast.size() - 1]->children[1]->computedType;
		}
	}
	std::string & resultType = lhsType;
	bool areTypesCompatible = true;
	if (lhsType != rhsType)
	{
		if (EXTRA_COMPATIBLE_TYPES.find(lhsType) == EXTRA_COMPATIBLE_TYPES.end())
		{
			if (EXTRA_COMPATIBLE_TYPES.find(rhsType) == EXTRA_COMPATIBLE_TYPES.end())
			{
				areTypesCompatible = false;
			}
			else
			{
				std::unordered_set<std::string> & rhsExtraCompatibleTypes = EXTRA_COMPATIBLE_TYPES.at(rhsType);
				if (rhsExtraCompatibleTypes.find(lhsType) == rhsExtraCompatibleTypes.end())
				{
					areTypesCompatible = false;
				}
				else
				{
					resultType = rhsType;
				}
			}
		}
		else
		{
			std::unordered_set<std::string> & lhsExtraCompatibleTypes = EXTRA_COMPATIBLE_TYPES.at(lhsType);
			if (lhsExtraCompatibleTypes.find(rhsType) == lhsExtraCompatibleTypes.end())
			{
				areTypesCompatible = false;
			}
		}
	}
	if (!areTypesCompatible)
	{
		PrintErrorMessage(
			"Cannot subtract \"" + lhs + "\"" + "(" + "\"" + lhsType + "\"" + " type" + ")"
			+ " with " + "\"" + rhs + "\"" + "(" + "\"" + rhsType + "\"" + " type" + ")" + "\n");

		return false;
	}
	if (identifiersExists)
	{
		m_ast[m_ast.size() - 2]->type = TokenConstant::Name::IDENTIFIER;
		m_ast[m_ast.size() - 2]->computedType = resultType;
	}
	else
	{
		std::string operationResult;
		std::string errorMessage;
		if (!Calculator::Subtract(lhs, rhs, resultType, operationResult, errorMessage))
		{
			PrintErrorMessage(errorMessage);

			return false;
		}
		m_ast[m_ast.size() - 2]->type = resultType;
		m_ast[m_ast.size() - 2]->computedType = resultType;
		m_ast[m_ast.size() - 2]->stringValue = operationResult;
		m_ast[m_ast.size() - 2]->children.clear();
		m_ast[m_ast.size() - 1]->children.clear();
	}

	return true;
}

bool LLParser::SynthesisMultiply()
{
	bool identifiersExists = false;
	std::string lhsType = m_ast[m_ast.size() - 2]->type;
	std::string & lhs = m_ast[m_ast.size() - 2]->stringValue;
	if (lhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (m_ast[m_ast.size() - 2]->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByName(lhs, symbolTableRow);
			lhsType = symbolTableRow.type;
		}
		else
		{
			lhsType = m_ast[m_ast.size() - 2]->computedType;
		}
	}
	std::string rhsType = m_ast[m_ast.size() - 1]->children[1]->type;
	std::string & rhs = m_ast[m_ast.size() - 1]->children[1]->stringValue;
	if (rhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (m_ast[m_ast.size() - 1]->children[1]->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByName(rhs, symbolTableRow);
			rhsType = symbolTableRow.type;
		}
		else
		{
			rhsType = m_ast[m_ast.size() - 1]->children[1]->computedType;
		}
	}
	std::string & resultType = lhsType;
	bool areTypesCompatible = true;
	if (lhsType != rhsType)
	{
		if (EXTRA_COMPATIBLE_TYPES.find(lhsType) == EXTRA_COMPATIBLE_TYPES.end())
		{
			if (EXTRA_COMPATIBLE_TYPES.find(rhsType) == EXTRA_COMPATIBLE_TYPES.end())
			{
				areTypesCompatible = false;
			}
			else
			{
				std::unordered_set<std::string> & rhsExtraCompatibleTypes = EXTRA_COMPATIBLE_TYPES.at(rhsType);
				if (rhsExtraCompatibleTypes.find(lhsType) == rhsExtraCompatibleTypes.end())
				{
					areTypesCompatible = false;
				}
				else
				{
					resultType = rhsType;
				}
			}
		}
		else
		{
			std::unordered_set<std::string> & lhsExtraCompatibleTypes = EXTRA_COMPATIBLE_TYPES.at(lhsType);
			if (lhsExtraCompatibleTypes.find(rhsType) == lhsExtraCompatibleTypes.end())
			{
				areTypesCompatible = false;
			}
		}
	}
	if (!areTypesCompatible)
	{
		PrintErrorMessage(
			"Cannot multiply \"" + lhs + "\"" + "(" + "\"" + lhsType + "\"" + " type" + ")"
			+ " with " + "\"" + rhs + "\"" + "(" + "\"" + rhsType + "\"" + " type" + ")" + "\n");

		return false;
	}

	if (identifiersExists)
	{
		m_ast[m_ast.size() - 2]->type = TokenConstant::Name::IDENTIFIER;
		m_ast[m_ast.size() - 2]->computedType = resultType;
	}
	else
	{
		std::string operationResult;
		std::string errorMessage;
		if (!Calculator::Multiply(lhs, rhs, resultType, operationResult, errorMessage))
		{
			PrintErrorMessage(errorMessage);

			return false;
		}
		m_ast[m_ast.size() - 2]->type = resultType;
		m_ast[m_ast.size() - 2]->computedType = resultType;
		m_ast[m_ast.size() - 2]->stringValue = operationResult;
		m_ast[m_ast.size() - 2]->children.clear();
		m_ast[m_ast.size() - 1]->children.clear();
	}

	return true;
}

bool LLParser::SynthesisIntegerDivision()
{
	bool identifiersExists = false;
	std::string lhsType = m_ast[m_ast.size() - 2]->type;
	std::string & lhs = m_ast[m_ast.size() - 2]->stringValue;
	if (lhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (m_ast[m_ast.size() - 2]->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByName(lhs, symbolTableRow);
			lhsType = symbolTableRow.type;
		}
		else
		{
			lhsType = m_ast[m_ast.size() - 2]->computedType;
		}
	}
	std::string rhsType = m_ast[m_ast.size() - 1]->children[1]->type;
	std::string & rhs = m_ast[m_ast.size() - 1]->children[1]->stringValue;
	if (rhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (m_ast[m_ast.size() - 1]->children[1]->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByName(rhs, symbolTableRow);
			rhsType = symbolTableRow.type;
		}
		else
		{
			rhsType = m_ast[m_ast.size() - 1]->children[1]->computedType;
		}
	}
	bool areTypesCompatible = true;
	if (lhsType != rhsType)
	{
		if (EXTRA_COMPATIBLE_TYPES.find(lhsType) == EXTRA_COMPATIBLE_TYPES.end())
		{
			if (EXTRA_COMPATIBLE_TYPES.find(rhsType) == EXTRA_COMPATIBLE_TYPES.end())
			{
				areTypesCompatible = false;
			}
			else
			{
				std::unordered_set<std::string> & rhsExtraCompatibleTypes = EXTRA_COMPATIBLE_TYPES.at(rhsType);
				if (rhsExtraCompatibleTypes.find(lhsType) == rhsExtraCompatibleTypes.end())
				{
					areTypesCompatible = false;
				}
			}
		}
		else
		{
			std::unordered_set<std::string> & lhsExtraCompatibleTypes = EXTRA_COMPATIBLE_TYPES.at(lhsType);
			if (lhsExtraCompatibleTypes.find(rhsType) == lhsExtraCompatibleTypes.end())
			{
				areTypesCompatible = false;
			}
		}
	}
	if (!areTypesCompatible)
	{
		PrintErrorMessage(
			"Cannot integer divide \"" + lhs + "\"" + "(" + "\"" + lhsType + "\"" + " type" + ")"
			+ " with " + "\"" + rhs + "\"" + "(" + "\"" + rhsType + "\"" + " type" + ")" + "\n");

		return false;
	}

	if (identifiersExists)
	{
		m_ast[m_ast.size() - 2]->type = TokenConstant::Name::IDENTIFIER;
		m_ast[m_ast.size() - 2]->computedType = TokenConstant::CoreType::Number::INTEGER;
	}
	else
	{
		std::string operationResult;
		std::string errorMessage;
		if (!Calculator::IntegerDivision(lhs, rhs, TokenConstant::CoreType::Number::INTEGER, operationResult, errorMessage))
		{
			PrintErrorMessage(errorMessage);

			return false;
		}
		m_ast[m_ast.size() - 2]->type = TokenConstant::CoreType::Number::INTEGER;
		m_ast[m_ast.size() - 2]->computedType = TokenConstant::CoreType::Number::INTEGER;
		m_ast[m_ast.size() - 2]->stringValue = operationResult;
		m_ast[m_ast.size() - 2]->children.clear();
		m_ast[m_ast.size() - 1]->children.clear();
	}

	return true;
}

void LLParser::PrintColoredMessage(std::string const & message, std::string const & colorCode) const
{
	std::cout << "\033[" + colorCode + "m";
	std::cout << message;
	std::cout << "\033[m";
}

void LLParser::PrintWarningMessage(std::string const & message) const
{
	PrintColoredMessage("Warning: " + message, "33");
}

void LLParser::PrintErrorMessage(std::string const & message) const
{
	PrintColoredMessage("Error: " + message, "31");
}

bool LLParser::SynthesisDivision()
{
	bool identifiersExists = false;
	std::string lhsType = m_ast[m_ast.size() - 2]->type;
	std::string & lhs = m_ast[m_ast.size() - 2]->stringValue;
	if (lhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (m_ast[m_ast.size() - 2]->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByName(lhs, symbolTableRow);
			lhsType = symbolTableRow.type;
		}
		else
		{
			lhsType = m_ast[m_ast.size() - 2]->computedType;
		}
	}
	std::string rhsType = m_ast[m_ast.size() - 1]->children[1]->type;
	std::string & rhs = m_ast[m_ast.size() - 1]->children[1]->stringValue;
	if (rhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (m_ast[m_ast.size() - 1]->children[1]->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByName(rhs, symbolTableRow);
			rhsType = symbolTableRow.type;
		}
		else
		{
			rhsType = m_ast[m_ast.size() - 1]->children[1]->computedType;
		}
	}
	bool areTypesCompatible = true;
	if (lhsType != rhsType)
	{
		if (EXTRA_COMPATIBLE_TYPES.find(lhsType) == EXTRA_COMPATIBLE_TYPES.end())
		{
			if (EXTRA_COMPATIBLE_TYPES.find(rhsType) == EXTRA_COMPATIBLE_TYPES.end())
			{
				areTypesCompatible = false;
			}
			else
			{
				std::unordered_set<std::string> & rhsExtraCompatibleTypes = EXTRA_COMPATIBLE_TYPES.at(rhsType);
				if (rhsExtraCompatibleTypes.find(lhsType) == rhsExtraCompatibleTypes.end())
				{
					areTypesCompatible = false;
				}
			}
		}
		else
		{
			std::unordered_set<std::string> & lhsExtraCompatibleTypes = EXTRA_COMPATIBLE_TYPES.at(lhsType);
			if (lhsExtraCompatibleTypes.find(rhsType) == lhsExtraCompatibleTypes.end())
			{
				areTypesCompatible = false;
			}
		}
	}
	if (!areTypesCompatible)
	{
		PrintErrorMessage(
			"Cannot divide \"" + lhs + "\"" + "(" + "\"" + lhsType + "\"" + " type" + ")"
			+ " with " + "\"" + rhs + "\"" + "(" + "\"" + rhsType + "\"" + " type" + ")" + "\n");

		return false;
	}

	if (identifiersExists)
	{
		m_ast[m_ast.size() - 2]->type = TokenConstant::Name::IDENTIFIER;
		m_ast[m_ast.size() - 2]->computedType = TokenConstant::CoreType::Number::FLOAT;
	}
	else
	{
		std::string operationResult;
		std::string errorMessage;
		if (!Calculator::Division(lhs, rhs, TokenConstant::CoreType::Number::FLOAT, operationResult, errorMessage))
		{
			PrintErrorMessage(errorMessage);

			return false;
		}
		m_ast[m_ast.size() - 2]->type = TokenConstant::CoreType::Number::FLOAT;
		m_ast[m_ast.size() - 2]->computedType = TokenConstant::CoreType::Number::FLOAT;
		m_ast[m_ast.size() - 2]->stringValue = operationResult;
		m_ast[m_ast.size() - 2]->children.clear();
		m_ast[m_ast.size() - 1]->children.clear();
	}

	return true;
}

bool LLParser::SynthesisModulus()
{
	bool identifiersExists = false;
	std::string lhsType = m_ast[m_ast.size() - 2]->type;
	std::string & lhs = m_ast[m_ast.size() - 2]->stringValue;
	if (lhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (m_ast[m_ast.size() - 2]->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByName(lhs, symbolTableRow);
			lhsType = symbolTableRow.type;
		}
		else
		{
			lhsType = m_ast[m_ast.size() - 2]->computedType;
		}
	}
	std::string rhsType = m_ast[m_ast.size() - 1]->children[1]->type;
	std::string & rhs = m_ast[m_ast.size() - 1]->children[1]->stringValue;
	if (rhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (m_ast[m_ast.size() - 1]->children[1]->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByName(rhs, symbolTableRow);
			rhsType = symbolTableRow.type;
		}
		else
		{
			rhsType = m_ast[m_ast.size() - 1]->children[1]->computedType;
		}
	}
	std::string & resultType = lhsType;
	bool areTypesCompatible = true;
	if (lhsType != rhsType)
	{
		if (EXTRA_COMPATIBLE_TYPES.find(lhsType) == EXTRA_COMPATIBLE_TYPES.end())
		{
			if (EXTRA_COMPATIBLE_TYPES.find(rhsType) == EXTRA_COMPATIBLE_TYPES.end())
			{
				areTypesCompatible = false;
			}
			else
			{
				std::unordered_set<std::string> & rhsExtraCompatibleTypes = EXTRA_COMPATIBLE_TYPES.at(rhsType);
				if (rhsExtraCompatibleTypes.find(lhsType) == rhsExtraCompatibleTypes.end())
				{
					areTypesCompatible = false;
				}
				else
				{
					resultType = rhsType;
				}
			}
		}
		else
		{
			std::unordered_set<std::string> & lhsExtraCompatibleTypes = EXTRA_COMPATIBLE_TYPES.at(lhsType);
			if (lhsExtraCompatibleTypes.find(rhsType) == lhsExtraCompatibleTypes.end())
			{
				areTypesCompatible = false;
			}
		}
	}
	if (!areTypesCompatible)
	{
		PrintErrorMessage(
			"Cannot module \"" + lhs + "\"" + "(" + "\"" + lhsType + "\"" + " type" + ")"
			+ " with " + "\"" + rhs + "\"" + "(" + "\"" + rhsType + "\"" + " type" + ")" + "\n");

		return false;
	}

	if (identifiersExists)
	{
		m_ast[m_ast.size() - 2]->type = TokenConstant::Name::IDENTIFIER;
		m_ast[m_ast.size() - 2]->computedType = resultType;
	}
	else
	{
		std::string operationResult;
		std::string errorMessage;
		if (!Calculator::Modulus(lhs, rhs, resultType, operationResult, errorMessage))
		{
			PrintErrorMessage(errorMessage);

			return false;
		}
		m_ast[m_ast.size() - 2]->type = resultType;
		m_ast[m_ast.size() - 2]->computedType = resultType;
		m_ast[m_ast.size() - 2]->stringValue = operationResult;
		m_ast[m_ast.size() - 2]->children.clear();
		m_ast[m_ast.size() - 1]->children.clear();
	}

	return true;
}

bool LLParser::CheckVariableTypeWithAssignmentRightHandTypeForEquality()
{
	std::string variableType = m_ast[m_ast.size() - 3]->children.front()->stringValue;
	std::string & variableName = m_ast[m_ast.size() - 3]->children[1]->stringValue;
	std::string rightHandType = m_ast[m_ast.size() - 1]->type;
	std::string & rightHandValue = m_ast[m_ast.size() - 1]->stringValue;
	if (rightHandType == TokenConstant::Name::IDENTIFIER)
	{
		if (m_ast[m_ast.size() - 1]->children[1]->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByName(rightHandValue, symbolTableRow);
			rightHandType = symbolTableRow.type;
		}
		else
		{
			rightHandType = m_ast[m_ast.size() - 1]->computedType;
		}
	}
	bool areTypesCompatible = true;
	if (variableType != rightHandType)
	{
		if (EXTRA_COMPATIBLE_TYPES.find(variableType) == EXTRA_COMPATIBLE_TYPES.end())
		{
			areTypesCompatible = false;
		}
		else
		{
			std::unordered_set<std::string> & variableExtraCompatibleTypes = EXTRA_COMPATIBLE_TYPES.at(variableType);
			if (variableExtraCompatibleTypes.find(rightHandType) == variableExtraCompatibleTypes.end())
			{
				areTypesCompatible = false;
			}
		}
	}
	if (!areTypesCompatible)
	{
		PrintErrorMessage(
			"Cannot set value \"" + rightHandValue + "\"" + "(" + "\"" + rightHandType + "\"" + " type" + ")"
			+ " to variable " + "\"" + variableName + "\"" + "(" + "\"" + variableType + "\"" + " type" + ")" + "\n");
	}

	return areTypesCompatible;
}

bool LLParser::CheckIdentifierTypeWithAssignmentRightHandTypeForEquality()
{
	std::string & variableName = m_ast[m_ast.size() - 3]->stringValue;
	SymbolTableRow symbolTableRow;
	m_symbolTable.GetSymbolTableRowByName(variableName, symbolTableRow);
	std::string & variableType = symbolTableRow.type;
	std::string rightHandType = m_ast[m_ast.size() - 1]->type;
	std::string & rightHandValue = m_ast[m_ast.size() - 1]->stringValue;
	if (rightHandType == TokenConstant::Name::IDENTIFIER)
	{
		if (m_ast[m_ast.size() - 1]->children[1]->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByName(rightHandValue, symbolTableRow);
			rightHandType = symbolTableRow.type;
		}
		else
		{
			rightHandType = m_ast[m_ast.size() - 1]->computedType;
		}
	}

	bool areTypesCompatible = true;
	if (variableType != rightHandType)
	{
		if (EXTRA_COMPATIBLE_TYPES.find(variableType) == EXTRA_COMPATIBLE_TYPES.end())
		{
			areTypesCompatible = false;
		}
		else
		{
			std::unordered_set<std::string> & variableExtraCompatibleTypes = EXTRA_COMPATIBLE_TYPES.at(variableType);
			if (variableExtraCompatibleTypes.find(rightHandType) == variableExtraCompatibleTypes.end())
			{
				areTypesCompatible = false;
			}
		}
	}
	if (!areTypesCompatible)
	{
		PrintErrorMessage(
			"Cannot set value \"" + rightHandValue + "\"" + "(" + "\"" + rightHandType + "\"" + " type" + ")"
			+ " to variable " + "\"" + variableName + "\"" + "(" + "\"" + variableType + "\"" + " type" + ")" + "\n");
	}

	return areTypesCompatible;
}

bool LLParser::SynthesisType()
{
	m_ast[m_ast.size() - 1]->type = m_ast[m_ast.size() - 1]->children.front()->type;
	m_ast[m_ast.size() - 1]->computedType = m_ast[m_ast.size() - 1]->children.front()->computedType;

	return true;
}
