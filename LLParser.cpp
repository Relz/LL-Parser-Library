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
#include <llvm/IR/DataLayout.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

LLParser::LLParser(std::string const & ruleFileName)
	: m_llTableBuilder(ruleFileName)
{
	m_module = std::make_unique<llvm::Module>("Main", m_context);
	m_dataLayout = std::make_unique<llvm::DataLayout>(m_module.get());
	std::vector<llvm::Type *> mainFunctionArgumentsTypes;
	m_mainFunctionType = llvm::FunctionType::get(llvm::Type::getInt32Ty(m_context), mainFunctionArgumentsTypes, false);
	m_mainFunction = llvm::Function::Create(m_mainFunctionType, llvm::GlobalValue::ExternalLinkage, "main", m_module.get());
	m_mainBlock = llvm::BasicBlock::Create(m_context, "main block", m_mainFunction, 0);
	m_builder = new llvm::IRBuilder(m_context);
	m_builder->SetInsertPoint(m_mainBlock);
	m_previousBlocks.push(m_mainBlock);
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
		std::cout << "\033[1;30;42m" << "--------------- Recode ---------------" << "\033[0m" << std::endl;
		PrintTokenInformations(tokenInformations, 0, tokenInformations.size(), "32");
		std::cout << "\n";
		std::cout << "\033[1;30;42m" << "--------------------------------------" << "\033[0m" << std::endl;
		std::cout << "         ⬇         ⬇         ⬇        " << std::endl;

		m_builder->CreateRet(LlvmHelper::CreateInteger32Constant(m_context, 0));
		std::cout << "\033[1;30;44m" << "------------ LLVM-IR Code ------------" << "\033[0m" << std::endl;
		llvm::outs() << "\033[34m";
		m_module->print(llvm::outs(), nullptr);
		std::cout << "\033[1;30;44m" << "--------------------------------------" << "\033[0m" << std::endl;
		std::cout << "         ⬇         ⬇         ⬇        " << std::endl;

		std::string errStr;
		llvm::ExecutionEngine * engine = llvm::EngineBuilder(std::move(m_module)).setErrorStr(&errStr).create();
		engine->finalizeObject();
		std::cout << "\033[1;30;47m" << "-------- Executed LLVM-IR Code -------" << "\033[0m" << std::endl;
		std::cout << "\033[37m";
		engine->runFunction(m_mainFunction, std::vector<llvm::GenericValue>());
		std::cout << "\033[1;30;47m" << "--------------------------------------" << "\033[0m" << std::endl;
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
	return it->empty() ? -1 : it->at(name);
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

void LLParser::ComputeDimensions(AstNode * extendedType, std::vector<unsigned int> & dimensions)
{
	if (extendedType->children.empty())
	{
		return;
	}
	std::vector<AstNode*> const & dimensionsNodes = extendedType->children.back()->children;
	for (AstNode * dimensionNode : dimensionsNodes)
	{
		dimensions.emplace_back(stoi(dimensionNode->stringValue));
	}
}

bool LLParser::AddVariableToScope()
{
	std::vector<AstNode*> & extendedType = m_ast[m_ast.size() - 3]->children;
	std::string & variableType = extendedType[0]->stringValue;
	std::string arraySizeString;
	if (variableType.empty())
	{
		variableType = extendedType.front()->children.front()->stringValue;
		arraySizeString = extendedType.front()->children.back()->children.front()->stringValue;
	}
	std::string & variableName = extendedType[1]->stringValue;
	std::vector<unsigned int> dimensions;
	ComputeDimensions(extendedType.front(), dimensions);
	llvm::Type * llvmType = LlvmHelper::CreateType(m_context, variableType, arraySizeString);
	llvm::AllocaInst * llvmAllocaInst = nullptr;
	if (variableType == TokenConstant::CoreType::Number::FLOAT && m_ast.back()->computedType == TokenConstant::CoreType::Number::INTEGER)
	{
		m_ast.back()->llvmValue = LlvmHelper::ConvertToFloat(m_builder, m_ast.back()->llvmValue);
	}
	if (arraySizeString.empty())
	{
		llvmAllocaInst = m_builder->CreateAlloca(llvmType, nullptr, "(" + variableName + ")" + "_pointer");
		m_builder->CreateStore(m_ast.back()->llvmValue, llvmAllocaInst);
	}
	else
	{
		int arraySize = stoi(arraySizeString);
		llvm::Value * arraySizeValue = LlvmHelper::CreateInteger32Constant(m_context, arraySize);
		llvmAllocaInst = m_builder->CreateAlloca(llvmType, arraySizeValue, "(" + variableName + ")" + "_pointer");
		CreateLlvmArrayAssignFunction(llvmAllocaInst, variableName, arraySize);
	}
	m_scopes.back()[variableName] = m_symbolTable.CreateRow(variableType, variableName, llvmAllocaInst, dimensions);

	return true;
}

bool LLParser::UpdateVariableInScope()
{
	std::string & variableName = m_ast[m_ast.size() - 3]->stringValue;

	SymbolTableRow symbolTableRow;
	m_symbolTable.GetSymbolTableRowByRowIndex(FindRowIndexInScopeByName(variableName), symbolTableRow);
	m_builder->CreateStore(m_ast.back()->llvmValue, symbolTableRow.llvmAllocaInst);

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
	std::string const & identifierNameToCheck = m_ast.back()->stringValue.empty() && !m_ast.back()->children.empty()
			? m_ast.back()->children.front()->stringValue
			: m_ast.back()->stringValue;
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
		if (lhsNode->computedType != TokenConstant::Name::IDENTIFIER)
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
		if (rhsNode->computedType != TokenConstant::Name::IDENTIFIER)
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
		lhsNode->type = TokenConstant::Name::IDENTIFIER;
		lhsNode->stringValue = "(" + lhs + " + " + rhs + ")";
		lhsNode->isTemporaryIdentifier = true;
		lhsNode->llvmValue = LlvmHelper::CreateAdd(m_builder, resultType, lhsNode->llvmValue, rhsNode->llvmValue, lhsNode->stringValue);
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
		if (lhsNode->computedType != TokenConstant::Name::IDENTIFIER)
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
		if (rhsNode->computedType != TokenConstant::Name::IDENTIFIER)
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
		lhsNode->stringValue = "(" + lhs + " - " + rhs + ")";
		lhsNode->isTemporaryIdentifier = true;
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
		if (lhsNode->computedType != TokenConstant::Name::IDENTIFIER)
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
		if (rhsNode->computedType != TokenConstant::Name::IDENTIFIER)
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
		lhsNode->stringValue = "(" + lhs + " * " + rhs + ")";
		lhsNode->isTemporaryIdentifier = true;
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
		if (lhsNode->computedType != TokenConstant::Name::IDENTIFIER)
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
		if (rhsNode->computedType != TokenConstant::Name::IDENTIFIER)
		{
			rhsType = rhsNode->computedType;
		}
	}
	std::string resultType;
	if (!AreTypesCompatible(lhsType, rhsType, resultType))
	{
		PrintErrorMessage(
			"Cannot integer divide \"" + lhs + "\"" + "(" + "\"" + lhsType + "\"" + " type" + ")"
			+ " with " + "\"" + rhs + "\"" + "(" + "\"" + rhsType + "\"" + " type" + ")" + "\n");

		return false;
	}

	if (identifiersExists)
	{
		resultType = TokenConstant::CoreType::Number::INTEGER;
		lhsNode->type = TokenConstant::Name::IDENTIFIER;
		lhsNode->stringValue = "(" + lhs + " // " + rhs + ")";
		lhsNode->isTemporaryIdentifier = true;
		lhsNode->llvmValue = LlvmHelper::CreateExactSDiv(m_builder, resultType, lhsNode->llvmValue, rhsNode->llvmValue, lhsNode->stringValue);
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
		if (lhsNode->computedType != TokenConstant::Name::IDENTIFIER)
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
		if (rhsNode->computedType != TokenConstant::Name::IDENTIFIER)
		{
			rhsType = rhsNode->computedType;
		}
	}
	std::string resultType;
	if (!AreTypesCompatible(lhsType, rhsType, resultType))
	{
		PrintErrorMessage(
			"Cannot divide \"" + lhs + "\"" + "(" + "\"" + lhsType + "\"" + " type" + ")"
			+ " with " + "\"" + rhs + "\"" + "(" + "\"" + rhsType + "\"" + " type" + ")" + "\n");

		return false;
	}

	if (identifiersExists)
	{
		resultType = TokenConstant::CoreType::Number::FLOAT;
		lhsNode->type = TokenConstant::Name::IDENTIFIER;
		lhsNode->stringValue = "(" + lhs + " / " + rhs + ")";
		lhsNode->isTemporaryIdentifier = true;
		lhsNode->llvmValue = LlvmHelper::CreateSDiv(m_builder, resultType, lhsNode->llvmValue, rhsNode->llvmValue, lhsNode->stringValue);
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
		if (lhsNode->computedType != TokenConstant::Name::IDENTIFIER)
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
		if (rhsNode->computedType != TokenConstant::Name::IDENTIFIER)
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
		lhsNode->isTemporaryIdentifier = true;
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
	std::vector<AstNode*> & variableDeclarationChildren = m_ast[m_ast.size() - 3]->children;
	std::string variableType = variableDeclarationChildren.front()->stringValue;
	std::string arraySizeString;
	if (variableType.empty())
	{
		variableType = TokenConstant::CoreType::Complex::ARRAY;
		arraySizeString = variableDeclarationChildren.front()->children.back()->children.front()->stringValue;
	}
	std::string & variableName = variableDeclarationChildren[1]->stringValue;
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
	else if (rightHandType == TokenConstant::Name::ARRAY_LITERAL)
	{
		AstNode * arrayLiteral = m_ast.back();
		if (arrayLiteral->children.size() != stoi(arraySizeString))
		{
			PrintErrorMessage(
				"Cannot set array literal with size " + std::to_string(arrayLiteral->children.size())
				+ " to array variable " + "\"" + variableName + "\"" + " with size " + std::string(arraySizeString) + "\n");

			return false;
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
	std::vector<AstNode *> & lastChildChildren = m_ast.back()->children;
	lastChildChildren.erase(lastChildChildren.begin());
	lastChildChildren.pop_back();

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

bool LLParser::ExpandChildrenLastChildren()
{
	std::vector<AstNode*> & children = m_ast.back()->children;
	std::vector<AstNode*> const & childrenLastChildred = children.back()->children;
	children.pop_back();
	children.insert(children.end(), childrenLastChildred.begin(), childrenLastChildred.end());

	return true;
}

bool LLParser::CreateLlvmStringLiteral()
{
	AstNode * astNode = m_ast.back();
	std::string stringLiteral;
	for (size_t i = 1; i < astNode->stringValue.length() - 1; ++i)
	{
		if (astNode->stringValue[i] == '\\')
		{
			char nextCharAfterSlash = astNode->stringValue[i + 1];
			if (nextCharAfterSlash == 'n')
			{
				stringLiteral += "\n";
			}
			else if (nextCharAfterSlash == 't')
			{
				stringLiteral += "\t";
			}
			++i;
		}
		else
		{
			stringLiteral += astNode->stringValue[i];
		}
	}
	std::string name = astNode->name + ": `" + stringLiteral + "`";
	astNode->llvmValue = m_builder->CreateGlobalStringPtr(stringLiteral, name);

	return true;
}

bool LLParser::CreateLlvmCharacterLiteral()
{
	AstNode * astNode = m_ast.back();
	astNode->llvmValue = LlvmHelper::CreateCharacterConstant(m_context, astNode->stringValue[1]);

	return true;
}

bool LLParser::CreateLlvmBooleanLiteral()
{
	AstNode * astNode = m_ast.back();
	astNode->llvmValue = LlvmHelper::CreateBooleanConstant(m_context, astNode->stringValue);

	return true;
}

void LLParser::ComputeArrayLiteralValues(std::vector<AstNode*> const & astNodes, std::vector<llvm::Constant*> & arrayLiteralValues)
{
	for (AstNode * astNode : astNodes)
	{
		arrayLiteralValues.emplace_back((llvm::Constant*)astNode->llvmValue);
	}
}

void LLParser::ComputeArrayLiteralName(AstNode * arrayLiteralNode, std::string & arrayLiteralName)
{
	std::string arrayLiteralString = "[";
	for (size_t i = 0; i < arrayLiteralNode->children.size(); ++i)
	{
		AstNode * arrayLiteralValue = arrayLiteralNode->children[i];
		arrayLiteralString += arrayLiteralValue->stringValue;
		if (i < arrayLiteralNode->children.size() - 1)
		{
			arrayLiteralString += ", ";
		}
	}
	arrayLiteralString += "]";
	arrayLiteralName = arrayLiteralNode->type + ": " + arrayLiteralString;
}

bool LLParser::CreateLlvmArrayLiteral()
{
	std::string const & arrayLiteralElementTypeString = m_ast[m_ast.size() - 3]->children.front()->children.front()->stringValue;
	std::vector<llvm::Constant*> arrayLiteralValues;
	ComputeArrayLiteralValues(m_ast.back()->children, arrayLiteralValues);
	llvm::ArrayType * arrayType = (llvm::ArrayType*)LlvmHelper::CreateType(m_context, arrayLiteralElementTypeString, std::to_string(arrayLiteralValues.size()));

	llvm::Constant * constant = llvm::ConstantArray::get(arrayType, arrayLiteralValues);
	std::string arrayLiteralName;
	ComputeArrayLiteralName(m_ast.back(), arrayLiteralName);
	auto * globalVariable = new llvm::GlobalVariable(
			*m_mainBlock->getParent()->getParent(), constant->getType(), true, llvm::GlobalValue::PrivateLinkage, constant, arrayLiteralName
	);
	globalVariable->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
	m_ast.back()->llvmValue = globalVariable;

	return true;
}

bool LLParser::CreateLlvmIntegerValue()
{
	AstNode * astNode = m_ast.back();
	astNode->llvmValue = LlvmHelper::CreateInteger32Constant(m_context, std::stoi(astNode->stringValue));

	return true;
}

bool LLParser::CreateLlvmFloatValue()
{
	AstNode * astNode = m_ast.back();
	astNode->llvmValue = LlvmHelper::CreateFloatConstant(m_context, std::stod(astNode->stringValue));

	return true;
}

bool LLParser::TryToLoadLlvmValueFromSymbolTable()
{
	AstNode * astNode = m_ast.back();
	SymbolTableRow symbolTableRow;
	if (astNode->stringValue.empty())
	{
		m_symbolTable.GetSymbolTableRowByRowIndex(FindRowIndexInScopeByName(astNode->children.front()->stringValue), symbolTableRow);
		std::string variableName = symbolTableRow.name + "[" + m_ast.back()->children.back()->stringValue + "]";
		llvm::Type * arrayType = LlvmHelper::CreateType(m_context, symbolTableRow.type, std::to_string(symbolTableRow.arrayInformation->dimensions.front()));
		llvm::Type * arrayElementType = LlvmHelper::CreateType(m_context, symbolTableRow.type);
		llvm::Value * index64Bit = m_builder->CreateSExtOrTrunc(m_ast.back()->children.back()->llvmValue, llvm::Type::getInt64Ty(m_context));
		llvm::Value * inBoundsGetElementPointer = m_builder->CreateInBoundsGEP(arrayType, symbolTableRow.llvmAllocaInst, { LlvmHelper::CreateInteger64Constant(m_context, 0), index64Bit }, "(" + variableName + ")_pointer");
		astNode->llvmValue = m_builder->CreateLoad(arrayElementType, inBoundsGetElementPointer, "(" + variableName + ")_value");
	}
	else
	{
		m_symbolTable.GetSymbolTableRowByRowIndex(FindRowIndexInScopeByName(astNode->stringValue), symbolTableRow);
		astNode->computedType = symbolTableRow.type;
		astNode->llvmValue = m_builder->CreateLoad(symbolTableRow.llvmAllocaInst, symbolTableRow.name + "_value");
	}

	return true;
}

bool LLParser::TryToReferenceLlvmValueFromSymbolTable()
{
	AstNode * astNode = m_ast.back();
	SymbolTableRow symbolTableRow;
	m_symbolTable.GetSymbolTableRowByRowIndex(FindRowIndexInScopeByName(astNode->stringValue), symbolTableRow);
	astNode->computedType = symbolTableRow.type;
	astNode->llvmValue = symbolTableRow.llvmAllocaInst;

	return true;
}

bool LLParser::RemoveComma()
{
	std::vector<AstNode*> & identifierListChildren = m_ast.back()->children;
	identifierListChildren.erase(identifierListChildren.begin());

	return true;
}

bool LLParser::RemovePredefinedFunctionReadOrWriteExtra()
{
	AstNode * container = m_ast.back();
	AstNode * function = container->children.front();
	AstNode * formatString = container->children[2];
	std::vector<AstNode*> const & parameters = container->children[3]->children;
	function->children.emplace_back(formatString);
	function->children.insert(function->children.end(), parameters.begin(), parameters.end());
	m_ast.back() = function;

	return true;
}

bool LLParser::CreateLlvmReadFunction()
{
	std::vector<AstNode*> & functionParameters = m_ast.back()->children;
	std::vector<llvm::Value *> arguments;
	for (AstNode * functionParameter : functionParameters)
	{
		arguments.emplace_back(functionParameter->llvmValue);
	}
	m_builder->CreateCall(LLParser::ScanfPrototype(m_context, m_module.get()), arguments);

	return true;
}

bool LLParser::CreateLlvmWriteFunction()
{
	std::vector<AstNode*> & functionParameters = m_ast.back()->children;
	std::vector<llvm::Value *> arguments;
	for (AstNode * functionParameter : functionParameters)
	{
		arguments.emplace_back(functionParameter->llvmValue);
	}
	m_builder->CreateCall(LLParser::PrintfPrototype(m_context, m_module.get()), arguments);

	return true;
}

bool LLParser::CreateLlvmArrayAssignFunction(llvm::Value * allocaInst, std::string const & variableName, int arraySize)
{
	llvm::Value * bitcasted = m_builder->CreateBitCast(allocaInst, llvm::Type::getInt8PtrTy(m_context), "(" + variableName + ")" + "_pointer_bitcasted");
	std::vector<llvm::Value*> arguments {
		bitcasted,
		m_builder->CreateBitCast(m_ast.back()->llvmValue, llvm::Type::getInt8PtrTy(m_context)),
		LlvmHelper::CreateInteger64Constant(m_context, m_dataLayout->getTypeAllocSize(allocaInst->getType()) * arraySize),
		LlvmHelper::CreateBooleanConstant(m_context, false)
	};

	m_builder->CreateCall(LLParser::MemcpyPrototype(m_context, m_module.get()), arguments);

	return true;
}

bool LLParser::SynthesisLastChildrenChildren()
{
	m_ast.back()->children = m_ast.back()->children.back()->children;

	return true;
}

bool LLParser::SynthesisLastChildren()
{
	m_ast.back() = m_ast.back()->children.back();

	return true;
}

bool LLParser::RemoveIfOrWhileStatementExtra()
{
	std::vector<AstNode*> & ifChildren = m_ast.back()->children;
	ifChildren.erase(ifChildren.begin());
	ifChildren.erase(ifChildren.begin());
	ifChildren.erase(ifChildren.begin() + 1);

	return true;
}

bool LLParser::CreateIfStatement()
{
	llvm::Value * condition = CreateCondition("if condition");
	llvm::BasicBlock * blockTrue = llvm::BasicBlock::Create(m_context, "block true", m_mainFunction);
	llvm::BasicBlock * blockFalse = llvm::BasicBlock::Create(m_context, "block false", m_mainFunction);
	m_blocksTrue.push(blockTrue);
	m_blocksFalse.push(blockFalse);

	m_builder->CreateCondBr(condition, blockTrue, blockFalse);

	return true;
}

bool LLParser::StartBlockTrue()
{
	m_builder->SetInsertPoint(m_blocksTrue.top());
	m_blocksTrue.pop();

	return true;
}

bool LLParser::StartBlockFalse()
{
	m_builder->SetInsertPoint(m_blocksFalse.top());
	m_blocksFalse.pop();

	return true;
}

bool LLParser::GotoPostIfStatementLabel()
{
	m_builder->CreateBr(m_previousBlocks.top());

	return true;
}

bool LLParser::StartBlockPrevious()
{
	m_builder->SetInsertPoint(m_previousBlocks.top());
	m_previousBlocks.pop();

	return true;
}

bool LLParser::SynthesisIfOrWhileCondition()
{
	m_ast.back()->children.front() = m_ast.back()->children.front()->children.front();

	return true;
}

bool LLParser::CreateBlockWhile()
{
	llvm::BasicBlock * blockWhileStatement = llvm::BasicBlock::Create(m_context, "while", m_mainFunction);
	m_whileBlocks.push(blockWhileStatement);

	return true;
}

bool LLParser::CreateWhileStatement()
{
	llvm::Value * condition = CreateCondition("while condition");
	llvm::BasicBlock * blockPostWhile = llvm::BasicBlock::Create(m_context, "post while", m_mainFunction);
	m_previousBlocks.push(blockPostWhile);

	m_builder->CreateCondBr(condition, m_whileBlocks.top(), blockPostWhile);

	return true;
}

bool LLParser::StartBlockWhile()
{
	m_builder->SetInsertPoint(m_whileBlocks.top());
	m_whileBlocks.pop();

	return true;
}

bool LLParser::CreateBlockPreWhile()
{
	llvm::BasicBlock * blockPreWhileStatement = llvm::BasicBlock::Create(m_context, "pre while", m_mainFunction);
	m_preWhileBlocks.push(blockPreWhileStatement);

	return true;
}

bool LLParser::GotoBlockPreWhile()
{
	m_builder->CreateBr(m_preWhileBlocks.top());

	return true;
}

bool LLParser::StartBlockPreWhile()
{
	m_builder->SetInsertPoint(m_preWhileBlocks.top());

	return true;
}

bool LLParser::SynthesisIfOrWhileConditionAndRemoveEmptyElse()
{
	SynthesisIfOrWhileCondition();
	m_ast.back()->children.pop_back();

	return true;
}

bool LLParser::SavePostIfStatementToPreviousBlocks()
{
	llvm::BasicBlock * blockPostIfStatement = llvm::BasicBlock::Create(m_context, "post if statement", m_mainFunction);
	m_previousBlocks.push(blockPostIfStatement);

	return true;
}

bool LLParser::EndBlockPreWhile()
{
	m_preWhileBlocks.pop();

	return true;
}

bool LLParser::ExpandArrayLiteral()
{
	if (m_ast.back()->children[1]->children.empty())
	{
		m_ast.back()->children.erase(m_ast.back()->children.begin());
		m_ast.back()->children.pop_back();
	}
	else
	{
		m_ast.back()->children = m_ast.back()->children[1]->children;
	}
	m_ast.back()->type = TokenConstant::Name::ARRAY_LITERAL;

	return true;
}

bool LLParser::SynthesisIdentifierPossibleArrayAccessing()
{
	m_ast.back()->children.back() = m_ast.back()->children.back()->children.front();

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

void LLParser::PrintTokenInformations(
	std::vector<TokenInformation> const & tokenInformations, size_t from, size_t to, std::string const & color
)
{
	std::cout << "\033[" + color + "m";
	StreamPosition outputStreamPosition;
	if (from > 0)
	{
		StreamString previousStreamString = tokenInformations.at(from - 1).GetTokenStreamString();
		outputStreamPosition = previousStreamString.position;
		outputStreamPosition.IncreaseColumn(previousStreamString.string.length());
	}

	for (
			size_t i = from; i < to && i < tokenInformations.size(); ++i
			)
	{
		TokenInformation const & tokenInformation = tokenInformations.at(i);
		StreamString const & streamString = tokenInformation.GetTokenStreamString();
		std::string const & streamStringString = streamString.string;
		StreamPosition const & streamPosition = streamString.position;
		long const & line = streamPosition.GetLine();
		long const & column = streamPosition.GetColumn();
		while (line > outputStreamPosition.GetLine())
		{
			std::cout << "\n";
			outputStreamPosition.IncreaseLine();
			outputStreamPosition.SetColumn(1);
		}
		char const indentCharacter = outputStreamPosition.GetColumn() == 1 ? '\t' : ' ';
		while (column > outputStreamPosition.GetColumn())
		{
			std::cout << indentCharacter;
			outputStreamPosition.IncreaseColumn();
		}
		for (
			char ch : streamStringString
				)
		{
			std::cout << ch;
			if (ch == '\n' || ch == '\r')
			{
				outputStreamPosition.IncreaseLine();
				outputStreamPosition.SetColumn(1);
			}
			else
			{
				outputStreamPosition.IncreaseColumn();
			}
		}
	}
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
	|| lhs == TokenConstant::Operator::Assignment::PLUS_ASSIGNMENT
	|| lhs == TokenConstant::Parentheses::ROUND_BRACKET.LEFT;
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

llvm::Value * LLParser::CreateCondition(std::string const & name)
{
	AstNode * expressionNode = m_ast[m_ast.size() - 2];

	llvm::Value * expression = expressionNode->llvmValue;
	if (expressionNode->computedType == TokenConstant::CoreType::Number::INTEGER)
	{
		expression = LlvmHelper::ConvertToFloat(m_builder, expression);
	}
	else if (expressionNode->computedType == TokenConstant::CoreType::BOOLEAN)
	{
		expression = LlvmHelper::ConvertToFloat(m_builder, expression);
	}
	else if (expressionNode->computedType != TokenConstant::CoreType::Number::FLOAT)
	{
		throw std::runtime_error("LLParser::CreateIfStatement: Unsupported type in expression \"" + expressionNode->computedType + "\"");
	}
	return m_builder->CreateFCmpONE(expression, LlvmHelper::CreateFloatConstant(m_context, 0.0), name);
}

llvm::Function * LLParser::PrintfPrototype(llvm::LLVMContext & context, llvm::Module * module)
{
	static std::vector<llvm::Type *> argumentsTypes { llvm::Type::getInt8PtrTy(context) };
	static llvm::FunctionType * type = llvm::FunctionType::get(llvm::Type::getInt32Ty(context), argumentsTypes, true);
	static llvm::Function * result = llvm::Function::Create(type, llvm::Function::ExternalLinkage, "printf", module);
	result->setCallingConv(llvm::CallingConv::C);

	return result;
}

llvm::Function * LLParser::ScanfPrototype(llvm::LLVMContext & context, llvm::Module * module)
{
	static std::vector<llvm::Type *> argumentsTypes { llvm::Type::getInt8PtrTy(context) };
	static llvm::FunctionType * type = llvm::FunctionType::get(llvm::Type::getInt32Ty(context), argumentsTypes, true);
	static llvm::Function * result = llvm::Function::Create(type, llvm::Function::ExternalLinkage, "scanf", module);
	result->setCallingConv(llvm::CallingConv::C);

	return result;
}

llvm::Function * LLParser::MemcpyPrototype(llvm::LLVMContext & context, llvm::Module * module)
{
	static std::vector<llvm::Type *> argumentsTypes {
		llvm::Type::getInt8PtrTy(context),
		llvm::Type::getInt8PtrTy(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt1Ty(context)
	};
	static llvm::FunctionType * type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), argumentsTypes, false);
	static llvm::Function * result = llvm::Function::Create(type, llvm::Function::ExternalLinkage, "memcpy", module);
	result->setCallingConv(llvm::CallingConv::C);

	return result;
}
