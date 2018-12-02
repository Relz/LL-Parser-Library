#include "LLParser.h"
#include "LLTableBuilderLibrary/Table/TableRow/TableRow.h"
#include "LLTableBuilderLibrary/TokenLibrary/TokenInformation/TokenInformation.h"
#include "LLTableBuilderLibrary/TokenLibrary/TokenExtensions/TokenExtensions.h"
#include "LexerLibrary/Lexer.h"
#include "LexerLibrary/TokenLibrary/TokenInformation/TokenInformation.h"
#include "Calculator/Calculator.h"
#include "LlvmHelper/LlvmHelper.h"
#include <string>
#include <functional>
#include <regex>
#include <unordered_set>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>

LLParser::LLParser(std::string const & ruleFileName)
	: m_llTableBuilder(ruleFileName)
{
	m_module = new llvm::Module("Main", m_context);
	std::vector<llvm::Type *> mainFunctionArgumentsTypes;
	m_mainFunctionType = llvm::FunctionType::get(llvm::Type::getInt32Ty(m_context), mainFunctionArgumentsTypes, false);
	m_mainFunction = llvm::Function::Create(m_mainFunctionType, llvm::GlobalValue::ExternalLinkage, "main", m_module);
	m_mainBlock = llvm::BasicBlock::Create(m_context, "main block", m_mainFunction, 0);
	m_builder = new llvm::IRBuilder(m_context);
	m_builder->SetInsertPoint(m_mainBlock);
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
	if (result)
	{
		m_builder->CreateRet(LlvmHelper::CreateIntegerConstant(m_context, 0));
		m_module->print(llvm::outs(), nullptr);
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
	if (actionName.empty() || IGNORED_ACTION_NAMES.find(actionName) != IGNORED_ACTION_NAMES.end())
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

unsigned int LLParser::FindRowIndexInScopeByName(std::string const & name) const
{
	auto const & it = std::find_if(
		m_scopes.begin(),
		m_scopes.end(),
		[&name](std::unordered_map<std::string, unsigned int> const & scope)
		{
			return scope.find(name) != scope.end();
		}
	);
	return it->at(name);
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
	std::string & variableName = extendedType[1]->stringValue;
	std::vector<unsigned int> dimensions;
	computeDimensions(dimensions);
	llvm::Type * llvmType = LlvmHelper::CreateType(m_context, variableType);
	llvm::AllocaInst * llvmAllocaInst = m_builder->CreateAlloca(llvmType, 0, variableName + "_pointer");
	m_builder->CreateStore(m_ast.back()->llvmValue, llvmAllocaInst);
	m_scopes.back()[variableName] = m_symbolTable.CreateRow(variableType, variableName, llvmAllocaInst, dimensions);

	return true;
}

bool LLParser::CheckIdentifierForAlreadyExisting() const
{
	std::string const & identifierNameToCheck = m_ast.back()->stringValue;
	for (std::unordered_map<std::string, unsigned int> const & scope : m_scopes)
	{
		if (scope.find(identifierNameToCheck) != scope.end())
		{
			PrintErrorMessage("Redeclaring identifier \"" + identifierNameToCheck + "\"" + "\n");

			return false;
		}
	}
	return true;
}

bool LLParser::CheckIdentifierForExisting() const
{
	std::string const & identifierNameToCheck = m_ast.back()->stringValue;
	for (std::unordered_map<std::string, unsigned int> const & scope : m_scopes)
	{
		if (scope.find(identifierNameToCheck) != scope.end())
		{
			return true;
		}
	}
	PrintErrorMessage("Undeclared identifier \"" + identifierNameToCheck + "\"" + "\n");

	return false;
}

bool LLParser::Synthesis()
{
	m_ast.back() = m_ast.back()->children.front();

	return true;
}

bool LLParser::SynthesisPlus()
{
	bool identifiersExists = false;
	AstNode * lhsNode = m_ast[m_ast.size() - 2];
	std::string lhsType = lhsNode->type;
	std::string & lhs = lhsNode->stringValue;
	if (lhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (lhsNode->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByRowIndex(FindRowIndexInScopeByName(lhs), symbolTableRow);
			lhsType = symbolTableRow.type;
			lhsNode->llvmValue = symbolTableRow.llvmAllocaInst;
		}
		else
		{
			lhsType = lhsNode->computedType;
		}
	}
	AstNode * rhsNode = m_ast.back()->children[1];
	std::string rhsType = rhsNode->type;
	std::string & rhs = rhsNode->stringValue;
	if (rhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (rhsNode->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByRowIndex(FindRowIndexInScopeByName(rhs), symbolTableRow);
			rhsType = symbolTableRow.type;
			rhsNode->llvmValue = symbolTableRow.llvmAllocaInst;
		}
		else
		{
			rhsType = rhsNode->computedType;
		}
	}
	std::string & resultType = lhsType;
	if (!AreTypesCompatible(lhsType, rhsType, resultType))
	{
		PrintErrorMessage(
			"Cannot add \"" + lhs + "\"" + "(" + "\"" + lhsType + "\"" + " type" + ")"
			+ " with " + "\"" + rhs + "\"" + "(" + "\"" + rhsType + "\"" + " type" + ")" + "\n");

		return false;
	}
	if (identifiersExists)
	{
		llvm::Value * lhsLlvmValue = lhsNode->type == TokenConstant::Name::IDENTIFIER
			? m_builder->CreateLoad(lhsNode->llvmValue, lhsNode->stringValue + "_value")
			: lhsNode->llvmValue;
		llvm::Value * rhsLlvmValue = rhsNode->type == TokenConstant::Name::IDENTIFIER
			? m_builder->CreateLoad(rhsNode->llvmValue, lhsNode->stringValue + "_value")
			: rhsNode->llvmValue;
		lhsNode->type = TokenConstant::Name::IDENTIFIER;
		lhsNode->stringValue = lhs + " + " + rhs;
		lhsNode->llvmValue = LlvmHelper::CreateAdd(m_builder, resultType, lhsLlvmValue, rhsLlvmValue, lhsNode->stringValue);
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
		lhsNode->type = resultType;
		lhsNode->llvmValue = LlvmHelper::CreateConstant(m_context, resultType, operationResult);
		lhsNode->stringValue = operationResult;
	}
	lhsNode->computedType = resultType;
	lhsNode->children.clear();
	m_ast.back()->children.clear();

	return true;
}

bool LLParser::SynthesisMinus()
{
	bool identifiersExists = false;
	AstNode * lhsNode = m_ast[m_ast.size() - 2];
	std::string lhsType = lhsNode->type;
	std::string lhs = lhsNode->stringValue;
	if (lhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (lhsNode->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByRowIndex(FindRowIndexInScopeByName(lhs), symbolTableRow);
			lhsType = symbolTableRow.type;
			lhsNode->llvmValue = symbolTableRow.llvmAllocaInst;
		}
		else
		{
			lhsType = lhsNode->computedType;
		}
	}
	AstNode * rhsNode = m_ast.back()->children[1];
	std::string rhsType = rhsNode->type;
	std::string & rhs = rhsNode->stringValue;
	if (rhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (rhsNode->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByRowIndex(FindRowIndexInScopeByName(rhs), symbolTableRow);
			rhsType = symbolTableRow.type;
			rhsNode->llvmValue = symbolTableRow.llvmAllocaInst;
		}
		else
		{
			rhsType = rhsNode->computedType;
		}
	}
	bool isUnaryMinus = IsUnaryMinus(lhs);
	if (isUnaryMinus)
	{
		lhsNode = CreateLiteralAstNode(TokenConstant::CoreType::Number::INTEGER, "0");
		lhsType = lhsNode->type;
		lhs = lhsNode->stringValue;

		m_ast.insert(m_ast.end() - 1, lhsNode);
	}
	std::string & resultType = lhsType;
	if (!AreTypesCompatible(lhsType, rhsType, resultType))
	{
		PrintErrorMessage(
			"Cannot subtract \"" + lhs + "\"" + "(" + "\"" + lhsType + "\"" + " type" + ")"
			+ " with " + "\"" + rhs + "\"" + "(" + "\"" + rhsType + "\"" + " type" + ")" + "\n");

		return false;
	}
	if (identifiersExists)
	{
		lhsNode->type = TokenConstant::Name::IDENTIFIER;
		lhsNode->stringValue = lhs + " - " + rhs;
		lhsNode->llvmValue = LlvmHelper::CreateSub(m_builder, resultType, lhsNode->llvmValue, rhsNode->llvmValue, lhsNode->stringValue);
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
		lhsNode->type = resultType;
		lhsNode->stringValue = operationResult;
		lhsNode->llvmValue = LlvmHelper::CreateConstant(m_context, resultType, operationResult);
	}
	lhsNode->computedType = resultType;
	lhsNode->children.clear();
	m_ast.back()->children.clear();
	if (isUnaryMinus)
	{
		m_ast.pop_back();
	}

	return true;
}

bool LLParser::SynthesisMultiply()
{
	bool identifiersExists = false;
	AstNode * lhsNode = m_ast[m_ast.size() - 2];
	std::string lhsType = lhsNode->type;
	std::string & lhs = lhsNode->stringValue;
	if (lhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (lhsNode->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByRowIndex(FindRowIndexInScopeByName(lhs), symbolTableRow);
			lhsType = symbolTableRow.type;
			lhsNode->llvmValue = symbolTableRow.llvmAllocaInst;
		}
		else
		{
			lhsType = lhsNode->computedType;
		}
	}
	AstNode * rhsNode = m_ast.back()->children[1];
	std::string rhsType = rhsNode->type;
	std::string & rhs = rhsNode->stringValue;
	if (rhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (rhsNode->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByRowIndex(FindRowIndexInScopeByName(rhs), symbolTableRow);
			rhsType = symbolTableRow.type;
			rhsNode->llvmValue = symbolTableRow.llvmAllocaInst;
		}
		else
		{
			rhsType = rhsNode->computedType;
		}
	}
	std::string & resultType = lhsType;
	if (!AreTypesCompatible(lhsType, rhsType, resultType))
	{
		PrintErrorMessage(
			"Cannot multiply \"" + lhs + "\"" + "(" + "\"" + lhsType + "\"" + " type" + ")"
			+ " with " + "\"" + rhs + "\"" + "(" + "\"" + rhsType + "\"" + " type" + ")" + "\n");

		return false;
	}

	if (identifiersExists)
	{
		lhsNode->type = TokenConstant::Name::IDENTIFIER;
		lhsNode->stringValue = lhs + " * " + rhs;
		lhsNode->llvmValue = LlvmHelper::CreateMul(m_builder, resultType, lhsNode->llvmValue, rhsNode->llvmValue, lhsNode->stringValue);
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
		lhsNode->type = resultType;
		lhsNode->llvmValue = LlvmHelper::CreateConstant(m_context, resultType, operationResult);
		lhsNode->stringValue = operationResult;
	}
	lhsNode->computedType = resultType;
	lhsNode->children.clear();
	m_ast.back()->children.clear();

	return true;
}

bool LLParser::SynthesisIntegerDivision()
{
	bool identifiersExists = false;
	AstNode * lhsNode = m_ast[m_ast.size() - 2];
	std::string lhsType = lhsNode->type;
	std::string & lhs = lhsNode->stringValue;
	if (lhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (lhsNode->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByRowIndex(FindRowIndexInScopeByName(lhs), symbolTableRow);
			lhsType = symbolTableRow.type;
			lhsNode->llvmValue = symbolTableRow.llvmAllocaInst;
		}
		else
		{
			lhsType = lhsNode->computedType;
		}
	}
	AstNode * rhsNode = m_ast.back()->children[1];
	std::string rhsType = rhsNode->type;
	std::string & rhs = rhsNode->stringValue;
	if (rhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (rhsNode->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByRowIndex(FindRowIndexInScopeByName(rhs), symbolTableRow);
			rhsType = symbolTableRow.type;
			rhsNode->llvmValue = symbolTableRow.llvmAllocaInst;
		}
		else
		{
			rhsType = rhsNode->computedType;
		}
	}
	std::string & resultType = lhsType;
	if (!AreTypesCompatible(lhsType, rhsType, resultType))
	{
		PrintErrorMessage(
			"Cannot integer divide \"" + lhs + "\"" + "(" + "\"" + lhsType + "\"" + " type" + ")"
			+ " with " + "\"" + rhs + "\"" + "(" + "\"" + rhsType + "\"" + " type" + ")" + "\n");

		return false;
	}

	if (identifiersExists)
	{
		lhsNode->type = TokenConstant::Name::IDENTIFIER;
		lhsNode->stringValue = lhs + " // " + rhs;
		lhsNode->llvmValue = LlvmHelper::CreateSDiv(m_builder, resultType, lhsNode->llvmValue, rhsNode->llvmValue, lhsNode->stringValue);
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
		lhsNode->type = TokenConstant::CoreType::Number::INTEGER;
		lhsNode->llvmValue = LlvmHelper::CreateConstant(m_context, TokenConstant::CoreType::Number::INTEGER, operationResult);
		lhsNode->stringValue = operationResult;
	}
	lhsNode->computedType = TokenConstant::CoreType::Number::INTEGER;
	lhsNode->children.clear();
	m_ast.back()->children.clear();

	return true;
}

bool LLParser::SynthesisDivision()
{
	bool identifiersExists = false;
	AstNode * lhsNode = m_ast[m_ast.size() - 2];
	std::string lhsType = lhsNode->type;
	std::string & lhs = lhsNode->stringValue;
	if (lhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (lhsNode->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByRowIndex(FindRowIndexInScopeByName(lhs), symbolTableRow);
			lhsType = symbolTableRow.type;
			lhsNode->llvmValue = symbolTableRow.llvmAllocaInst;
		}
		else
		{
			lhsType = lhsNode->computedType;
		}
	}
	AstNode * rhsNode = m_ast.back()->children[1];
	std::string rhsType = rhsNode->type;
	std::string & rhs = rhsNode->stringValue;
	if (rhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (rhsNode->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByRowIndex(FindRowIndexInScopeByName(rhs), symbolTableRow);
			rhsType = symbolTableRow.type;
			rhsNode->llvmValue = symbolTableRow.llvmAllocaInst;
		}
		else
		{
			rhsType = rhsNode->computedType;
		}
	}
	std::string & resultType = lhsType;
	if (!AreTypesCompatible(lhsType, rhsType, resultType))
	{
		PrintErrorMessage(
			"Cannot divide \"" + lhs + "\"" + "(" + "\"" + lhsType + "\"" + " type" + ")"
			+ " with " + "\"" + rhs + "\"" + "(" + "\"" + rhsType + "\"" + " type" + ")" + "\n");

		return false;
	}

	if (identifiersExists)
	{
		lhsNode->type = TokenConstant::Name::IDENTIFIER;
		lhsNode->stringValue = lhs + " / " + rhs;
		lhsNode->llvmValue = LlvmHelper::CreateExactSDiv(m_builder, resultType, lhsNode->llvmValue, rhsNode->llvmValue, lhsNode->stringValue);
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
		lhsNode->type = TokenConstant::CoreType::Number::FLOAT;
		lhsNode->llvmValue = LlvmHelper::CreateConstant(m_context, TokenConstant::CoreType::Number::FLOAT, operationResult);
		lhsNode->stringValue = operationResult;
	}
	lhsNode->computedType = TokenConstant::CoreType::Number::FLOAT;
	lhsNode->children.clear();
	m_ast.back()->children.clear();

	return true;
}

bool LLParser::SynthesisModulus()
{
	bool identifiersExists = false;
	AstNode * lhsNode = m_ast[m_ast.size() - 2];
	std::string lhsType = lhsNode->type;
	std::string & lhs = lhsNode->stringValue;
	if (lhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (lhsNode->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByRowIndex(FindRowIndexInScopeByName(lhs), symbolTableRow);
			lhsType = symbolTableRow.type;
			lhsNode->llvmValue = symbolTableRow.llvmAllocaInst;
		}
		else
		{
			lhsType = lhsNode->computedType;
		}
	}
	AstNode * rhsNode = m_ast.back()->children[1];
	std::string rhsType = rhsNode->type;
	std::string & rhs = rhsNode->stringValue;
	if (rhsType == TokenConstant::Name::IDENTIFIER)
	{
		identifiersExists = true;
		if (rhsNode->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByRowIndex(FindRowIndexInScopeByName(rhs), symbolTableRow);
			rhsType = symbolTableRow.type;
			rhsNode->llvmValue = symbolTableRow.llvmAllocaInst;
		}
		else
		{
			rhsType = rhsNode->computedType;
		}
	}
	std::string & resultType = lhsType;
	if (!AreTypesCompatible(lhsType, rhsType, resultType))
	{
		PrintErrorMessage(
			"Cannot module \"" + lhs + "\"" + "(" + "\"" + lhsType + "\"" + " type" + ")"
			+ " with " + "\"" + rhs + "\"" + "(" + "\"" + rhsType + "\"" + " type" + ")" + "\n");

		return false;
	}

	if (identifiersExists)
	{
		lhsNode->type = TokenConstant::Name::IDENTIFIER;
		lhsNode->stringValue = lhs + " % " + rhs;
		lhsNode->llvmValue = LlvmHelper::CreateSRem(m_builder, resultType, lhsNode->llvmValue, rhsNode->llvmValue, lhsNode->stringValue);
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
		lhsNode->type = resultType;
		lhsNode->llvmValue = LlvmHelper::CreateConstant(m_context, resultType, operationResult);
		lhsNode->stringValue = operationResult;
	}
	lhsNode->computedType = resultType;
	lhsNode->children.clear();
	m_ast.back()->children.clear();

	return true;
}

bool LLParser::CheckVariableTypeWithAssignmentRightHandTypeForEquality() const
{
	std::string variableType = m_ast[m_ast.size() - 3]->children.front()->stringValue;
	std::string & variableName = m_ast[m_ast.size() - 3]->children[1]->stringValue;
	std::string rightHandType = m_ast.back()->type;
	std::string & rightHandValue = m_ast.back()->stringValue;
	if (rightHandType == TokenConstant::Name::IDENTIFIER)
	{
		if (m_ast.back()->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByRowIndex(FindRowIndexInScopeByName(rightHandValue), symbolTableRow);
			rightHandType = symbolTableRow.type;
		}
		else
		{
			rightHandType = m_ast.back()->computedType;
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
			std::unordered_set<std::string> const & variableExtraCompatibleTypes = EXTRA_COMPATIBLE_TYPES.at(variableType);
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

bool LLParser::CheckIdentifierTypeWithAssignmentRightHandTypeForEquality() const
{
	std::string & variableName = m_ast[m_ast.size() - 3]->stringValue;
	SymbolTableRow symbolTableRow;
	m_symbolTable.GetSymbolTableRowByRowIndex(FindRowIndexInScopeByName(variableName), symbolTableRow);
	std::string & variableType = symbolTableRow.type;
	std::string rightHandType = m_ast.back()->type;
	std::string & rightHandValue = m_ast.back()->stringValue;
	if (rightHandType == TokenConstant::Name::IDENTIFIER)
	{
		if (m_ast.back()->computedType == TokenConstant::Name::IDENTIFIER)
		{
			SymbolTableRow symbolTableRow;
			m_symbolTable.GetSymbolTableRowByRowIndex(FindRowIndexInScopeByName(rightHandValue), symbolTableRow);
			rightHandType = symbolTableRow.type;
		}
		else
		{
			rightHandType = m_ast.back()->computedType;
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
			std::unordered_set<std::string> const & variableExtraCompatibleTypes = EXTRA_COMPATIBLE_TYPES.at(variableType);
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
	m_ast.back()->type = m_ast.back()->children.front()->type;
	m_ast.back()->computedType = m_ast.back()->children.front()->computedType;

	return true;
}

bool LLParser::RemoveBrackets()
{
	m_ast.back()->children.erase(m_ast.back()->children.begin());
	m_ast.back()->children.pop_back();

	return true;
}

bool LLParser::RemoveBracketsAndSynthesis()
{
	m_ast.back() = m_ast.back()->children[1];

	return true;
}

bool LLParser::RemoveIfRoundBrackets()
{
	std::vector<AstNode *> & ifKeyword = m_ast.back()->children;
	ifKeyword.erase(ifKeyword.begin() + 1);
	ifKeyword.erase(ifKeyword.begin() + 2);

	return true;
}

bool LLParser::RemoveSemicolon()
{
	m_ast.back() = m_ast.back()->children.front();

	return true;
}

bool LLParser::RemoveScopeBrackets()
{
	m_ast.back()->children.pop_back();
	m_ast.erase(m_ast.end() - 2);

	return true;
}

bool LLParser::ExpandStatementList()
{
	std::vector<AstNode*> const & children = m_ast.back()->children.back()->children;
	m_ast.back()->children.pop_back();
	for (AstNode* child : children)
	{
		m_ast.back()->children.emplace_back(child);
	}
	return true;
}

bool LLParser::CreateLlvmIntegerValue()
{
	AstNode * astNode = m_ast.back();
	astNode->llvmValue = llvm::ConstantInt::get(llvm::IntegerType::getInt32Ty(m_context), std::stoi(astNode->stringValue), true);

	return true;
}

bool LLParser::CreateLlvmFloatValue()
{
	AstNode * astNode = m_ast.back();
	astNode->llvmValue = llvm::ConstantFP::get(m_context, llvm::APFloat(std::stof(astNode->stringValue)));

	return true;
}

bool LLParser::abc()
{
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

bool LLParser::AreTypesCompatible(std::string const & lhsType, std::string const & rhsType, std::string & resultType)
{
	if (lhsType == rhsType)
	{
		resultType = lhsType;

		return true;
	}
	if (EXTRA_COMPATIBLE_TYPES.find(lhsType) == EXTRA_COMPATIBLE_TYPES.end())
	{
		if (EXTRA_COMPATIBLE_TYPES.find(rhsType) != EXTRA_COMPATIBLE_TYPES.end())
		{
			std::unordered_set<std::string> & rhsExtraCompatibleTypes = EXTRA_COMPATIBLE_TYPES.at(rhsType);

			if (rhsExtraCompatibleTypes.find(lhsType) == rhsExtraCompatibleTypes.end())
			{
				return false;
			}
			else
			{
				resultType = rhsType;

				return true;
			}
		}
	}
	else
	{
		std::unordered_set<std::string> & lhsExtraCompatibleTypes = EXTRA_COMPATIBLE_TYPES.at(lhsType);

		if (lhsExtraCompatibleTypes.find(rhsType) == lhsExtraCompatibleTypes.end())
		{
			return false;
		}
		else
		{
			resultType = lhsType;

			return true;
		}
	}
	return false;
}

bool LLParser::IsUnaryMinus(std::string const & lhs)
{
return lhs == TokenConstant::Operator::Assignment::ASSIGNMENT
	|| lhs == TokenConstant::Operator::Arithmetic::DIVISION
	|| lhs == TokenConstant::Operator::Assignment::DIVISION_ASSIGNMENT
	|| lhs == TokenConstant::Operator::Arithmetic::INTEGER_DIVISION
	|| lhs == TokenConstant::Operator::Assignment::INTEGER_DIVISION_ASSIGNMENT
	|| lhs == TokenConstant::Operator::Arithmetic::MINUS
	|| lhs == TokenConstant::Operator::Assignment::MINUS_ASSIGNMENT
	|| lhs == TokenConstant::Operator::Arithmetic::MODULUS
	|| lhs == TokenConstant::Operator::Assignment::MODULUS_ASSIGNMENT
	|| lhs == TokenConstant::Operator::Arithmetic::MULTIPLY
	|| lhs == TokenConstant::Operator::Assignment::MULTIPLY_ASSIGNMENT
	|| lhs == TokenConstant::Operator::Arithmetic::PLUS
	|| lhs == TokenConstant::Operator::Assignment::PLUS_ASSIGNMENT;
}

AstNode * LLParser::CreateLiteralAstNode(std::string const & type, std::string const & value)
{
	AstNode * result = new AstNode();
	result->name = type;
	result->type = type;
	result->computedType = type;
	result->stringValue = value;
	result->llvmValue = LlvmHelper::CreateConstant(m_context, type, value);

	return result;
}
