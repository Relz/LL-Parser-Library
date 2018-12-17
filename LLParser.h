#ifndef LLPARSERLIBRARY_LIBRARY_H
#define LLPARSERLIBRARY_LIBRARY_H

#include "LLTableBuilderLibrary/LLTableBuilder.h"
#include "AstNode/AstNode.h"
#include "SymbolTable/SymbolTable.h"
#include "LexerLibrary/TokenLibrary/Token.h"
#include "LLTableBuilderLibrary/TokenLibrary/TokenExtensions/TokenExtensions.h"
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>

class TokenInformation;

class LLParser
{
public:
	explicit LLParser(std::string const & ruleFileName);

	bool IsValid(
		std::string const & inputFileName,
		std::vector<TokenInformation> & tokenInformations,
		size_t & failIndex,
		std::unordered_set<Token> & expectedWords
	);

private:
	AstNode * CreateAstNode(std::string const & ruleName, unsigned int tokenCount);

	bool TryToCreateAstNode(std::string const & actionName);

	static bool ParseCreateAstNodeAction(std::string const & actionName, std::string & ruleName, unsigned int & tokenCount);
	bool ResolveActionName(std::string const & actionName) const;
	bool ResolveAstActionName(std::string const & actionName);

	unsigned int FindRowIndexInScopeByName(std::string const & name) const;

	bool CreateScopeAction();
	bool DestroyScopeAction();
	void computeDimensions(std::vector<unsigned int> & dimensions);
	bool AddVariableToScope();
	bool UpdateVariableInScope();
	bool CheckIdentifierForAlreadyExisting() const;
	bool CheckIdentifierForExisting() const;
	bool Synthesis();
	bool SynthesisPlus();
	bool SynthesisMinus();
	bool SynthesisMultiply();
	bool SynthesisIntegerDivision();
	bool SynthesisDivision();
	bool SynthesisModulus();
	bool CheckVariableTypeWithAssignmentRightHandTypeForEquality() const;
	bool CheckIdentifierTypeWithAssignmentRightHandTypeForEquality() const;
	bool SynthesisType();
	bool RemoveBrackets();
	bool RemoveIfRoundBrackets();
	bool RemoveSemicolon();
	bool RemoveScopeBrackets();
	bool ExpandChildrenLastChildren();
	bool RemoveBracketsAndSynthesis();
	bool RemoveComma();
	bool RemovePredefinedFunctionReadOrWriteExtra();
	bool CreateLllvmReadFunction();
	bool CreateLllvmWriteFunction();
	bool SynthesisLastChildrenChildren();
	bool RemoveElseKeyword();
	bool RemoveIfOrWhileStatementExtra();
	bool CreateIfStatement();
	bool GotoPostIfStatementLabel();
	bool StartBlockTrue();
	bool StartBlockFalse();
	bool StartBlockPrevious();
	bool EndBlockPrevious();
	bool CreateWhileStatement();
	bool SynthesisIfOrWhileCondition();
	bool CreateBlockWhile();
	bool StartBlockWhile();
	bool CreateBlockPreWhile();
	bool GotoBlockPreWhile();
	bool StartBlockPreWhile();
	bool SynthesisIfOrWhileConditionAndRemoveEmptyElse();
	bool SavePostIfStatementToPreviousBlocks();
	bool abc();

	bool CreateLlvmStringLiteral();
	bool CreateLlvmCharacterLiteral();
	bool CreateLlvmBooleanLiteral();
	bool CreateLlvmIntegerValue();
	bool CreateLlvmFloatValue();
	bool TryToLoadLlvmValueFromSymbolTable();
	bool TryToReferenceLlvmValueFromSymbolTable();

	void PrintTokenInformations(
		std::vector<TokenInformation> const & tokenInformations, size_t from, size_t to, std::string const & color
	);
	void PrintColoredMessage(std::string const & message, std::string const & colorCode) const;
	void PrintWarningMessage(std::string const & message) const;
	void PrintErrorMessage(std::string const & message) const;

	bool AreTypesCompatible(std::string const & lhsType, std::string const & rhsType, std::string & resultType);
	bool IsUnaryMinus(std::string const & lhs);
	AstNode * CreateLiteralAstNode(std::string const & type, std::string const & value);
	llvm::Value * CreateCondition(std::string const & name);

	static llvm::Function * PrintfPrototype(llvm::LLVMContext & context, llvm::Module * module);
	static llvm::Function * ScanfPrototype(llvm::LLVMContext & context, llvm::Module * module);

	std::unordered_map<std::string, std::function<bool()>> const ACTION_NAME_TO_ACTION_MAP {
		{ "Create scope", std::bind(&LLParser::CreateScopeAction, this) },
		{ "Destroy scope", std::bind(&LLParser::DestroyScopeAction, this) },
		{ "Add variable to scope", std::bind(&LLParser::AddVariableToScope, this) },
		{ "Update variable in scope", std::bind(&LLParser::UpdateVariableInScope, this) },
		{ "Check identifier for already existing", std::bind(&LLParser::CheckIdentifierForAlreadyExisting, this) },
		{ "Check identifier for existing", std::bind(&LLParser::CheckIdentifierForExisting, this) },
		{ "Synthesis", std::bind(&LLParser::Synthesis, this) },
		{ "Check variable type with AssignmentRightHand type for equality", std::bind(&LLParser::CheckVariableTypeWithAssignmentRightHandTypeForEquality, this) },
		{ "Check identifier type with AssignmentRightHand type for equality", std::bind(&LLParser::CheckIdentifierTypeWithAssignmentRightHandTypeForEquality, this) },
		{ "Create LLVM string literal", std::bind(&LLParser::CreateLlvmStringLiteral, this) },
		{ "Create LLVM character literal", std::bind(&LLParser::CreateLlvmCharacterLiteral, this) },
		{ "Create LLVM boolean literal", std::bind(&LLParser::CreateLlvmBooleanLiteral, this) },
		{ "Create llvm integer value", std::bind(&LLParser::CreateLlvmIntegerValue, this) },
		{ "Create llvm float value", std::bind(&LLParser::CreateLlvmFloatValue, this) },
		{ "Try to load LLVM value from symbol table", std::bind(&LLParser::TryToLoadLlvmValueFromSymbolTable, this) },
		{ "Try to reference LLVM value from symbol table", std::bind(&LLParser::TryToReferenceLlvmValueFromSymbolTable, this) },
		{ "Create LLVM read function", std::bind(&LLParser::CreateLllvmReadFunction, this) },
		{ "Create LLVM write function", std::bind(&LLParser::CreateLllvmWriteFunction, this) },
		{ "Create if statement", std::bind(&LLParser::CreateIfStatement, this) },
		{ "Start block true", std::bind(&LLParser::StartBlockTrue, this) },
		{ "Goto post if statement label", std::bind(&LLParser::GotoPostIfStatementLabel, this) },
		{ "Start block false", std::bind(&LLParser::StartBlockFalse, this) },
		{ "Start block previous", std::bind(&LLParser::StartBlockPrevious, this) },
		{ "End block previous", std::bind(&LLParser::EndBlockPrevious, this) },
		{ "Create while statement", std::bind(&LLParser::CreateWhileStatement, this) },
		{ "Create block while", std::bind(&LLParser::CreateBlockWhile, this) },
		{ "Start block while", std::bind(&LLParser::StartBlockWhile, this) },
		{ "Create block pre while", std::bind(&LLParser::CreateBlockPreWhile, this) },
		{ "Goto block pre while", std::bind(&LLParser::GotoBlockPreWhile, this) },
		{ "Start block pre while", std::bind(&LLParser::StartBlockPreWhile, this) },
		{ "Save post if statement to previous blocks", std::bind(&LLParser::SavePostIfStatementToPreviousBlocks, this) },

		{ "Synthesis Plus Integer", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Integer B", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Float", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Float B", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Identifier", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Identifier B", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus String", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus String B", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus String literal", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus String literal B", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Character literal", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Character literal B", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Character", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Character B", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus ArithmeticMinus", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus ArithmeticMultiply", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus ArithmeticDivision", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus ArithmeticIntegerDivision", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus ArithmeticModule", std::bind(&LLParser::SynthesisPlus, this) },

		{ "Synthesis Minus Integer", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Integer B", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Float", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Float B", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Identifier", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Identifier B", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus String", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus String B", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus String literal", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus String literal B", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Character literal", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Character literal B", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Character", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Character B", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus ArithmeticMinus", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus ArithmeticMultiply", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus ArithmeticDivision", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus ArithmeticIntegerDivision", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus ArithmeticModule", std::bind(&LLParser::SynthesisMinus, this) },

		{ "Synthesis Multiply Integer", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Integer B", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Float", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Float B", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Identifier", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Identifier B", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply String", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply String B", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply String literal", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply String literal B", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Character literal", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Character literal B", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Character", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Character B", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply ArithmeticMinus", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply ArithmeticMultiply", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply ArithmeticDivision", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply ArithmeticIntegerDivision", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply ArithmeticModule", std::bind(&LLParser::SynthesisMultiply, this) },

		{ "Synthesis Integer division Integer", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Integer B", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Float", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Float B", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Identifier", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Identifier B", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division String", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division String B", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division String literal", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division String literal B", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Character literal", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Character literal B", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Character", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Character B", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division ArithmeticMinus", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division ArithmeticMultiply", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division ArithmeticDivision", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division ArithmeticIntegerDivision", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division ArithmeticModule", std::bind(&LLParser::SynthesisIntegerDivision, this) },

		{ "Synthesis Division Integer", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Integer B", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Float", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Float B", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Identifier", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Identifier B", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division String", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division String B", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division String literal", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division String literal B", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Character literal", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Character literal B", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Character", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Character B", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division ArithmeticMinus", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division ArithmeticMultiply", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division ArithmeticDivision", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division ArithmeticIntegerDivision", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division ArithmeticModule", std::bind(&LLParser::SynthesisDivision, this) },

		{ "Synthesis Modulus Integer", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Integer B", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Float", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Float B", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Identifier", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Identifier B", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus String", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus String B", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus String literal", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus String literal B", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Character literal", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Character literal B", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Character", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Character B", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus ArithmeticMinus", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus ArithmeticMultiply", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus ArithmeticDivision", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus ArithmeticIntegerDivision", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus ArithmeticModule", std::bind(&LLParser::SynthesisModulus, this) },

		{ "Synthesis Integer A", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Float A", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Identifier A", std::bind(&LLParser::SynthesisType, this) },

		{ "Synthesis Integer B", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Float B", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Identifier B", std::bind(&LLParser::SynthesisType, this) },

		{ "Synthesis Integer C", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Float C", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Identifier C", std::bind(&LLParser::SynthesisType, this) },

		{ "Synthesis Integer D", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Float D", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Identifier D", std::bind(&LLParser::SynthesisType, this) },

		{ "Synthesis Integer E", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Float E", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Identifier E", std::bind(&LLParser::SynthesisType, this) },

		{ "Synthesis Integer F", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Float F", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Identifier F", std::bind(&LLParser::SynthesisType, this) },

		{ "Synthesis Left round bracket Integer Right round bracket", std::bind(&LLParser::RemoveBracketsAndSynthesis, this) },
		{ "Synthesis Left round bracket Float Right round bracket", std::bind(&LLParser::RemoveBracketsAndSynthesis, this) },
		{ "Synthesis Left round bracket Identifier Right round bracket", std::bind(&LLParser::RemoveBracketsAndSynthesis, this) },
		{ "Synthesis Left round bracket String Right round bracket", std::bind(&LLParser::RemoveBracketsAndSynthesis, this) },
		{ "Synthesis Left round bracket String literal Right round bracket", std::bind(&LLParser::RemoveBracketsAndSynthesis, this) },
		{ "Synthesis Left round bracket Character Right round bracket", std::bind(&LLParser::RemoveBracketsAndSynthesis, this) },
		{ "Synthesis Left round bracket Character literal Right round bracket", std::bind(&LLParser::RemoveBracketsAndSynthesis, this) },
		{ "Synthesis If keyword Left round bracket Identifier Right round bracket Statement", std::bind(&LLParser::RemoveIfRoundBrackets, this) },
		{ "Synthesis Assignment Semicolon", std::bind(&LLParser::RemoveSemicolon, this) },
		{ "Synthesis VariableDeclaration Semicolon", std::bind(&LLParser::RemoveSemicolon, this) },
		{ "Synthesis Left curly bracket Right curly bracket", std::bind(&LLParser::RemoveBrackets, this) },
		{ "Synthesis VariableDeclaration StatementList Right curly bracket", std::bind(&LLParser::RemoveScopeBrackets, this) },
		{ "Synthesis VariableDeclaration StatementList", std::bind(&LLParser::ExpandChildrenLastChildren, this) },
		{ "Synthesis Assignment StatementList", std::bind(&LLParser::ExpandChildrenLastChildren, this) },
		{ "Synthesis Left curly bracket StatementList Right curly bracket", std::bind(&LLParser::RemoveBrackets, this) },
		{ "Synthesis Left curly bracket VariableDeclaration Right curly bracket", std::bind(&LLParser::RemoveBrackets, this) },
		{ "Synthesis Identifier IdentifierListExtension", std::bind(&LLParser::ExpandChildrenLastChildren, this) },
		{ "Synthesis Identifier ReferencedIdentifierListExtension", std::bind(&LLParser::ExpandChildrenLastChildren, this) },
		{ "Synthesis Identifier ValuedIdentifierListExtension", std::bind(&LLParser::ExpandChildrenLastChildren, this) },
		{ "Synthesis Comma Identifier", std::bind(&LLParser::RemoveComma, this) },
		{ "Synthesis Read function Left round bracket String literal ReadExtra Right round bracket Semicolon", std::bind(&LLParser::RemovePredefinedFunctionReadOrWriteExtra, this) },
		{ "Synthesis Write function Left round bracket String literal WriteExtra Right round bracket Semicolon", std::bind(&LLParser::RemovePredefinedFunctionReadOrWriteExtra, this) },
		{ "Synthesis Comma IdentifierList", std::bind(&LLParser::SynthesisLastChildrenChildren, this) },
		{ "Synthesis Comma ReferencedIdentifierList", std::bind(&LLParser::SynthesisLastChildrenChildren, this) },
		{ "Synthesis Comma ValuedIdentifierList", std::bind(&LLParser::SynthesisLastChildrenChildren, this) },
		{ "Synthesis Write function Left round bracket String literal Right round bracket Semicolon", std::bind(&LLParser::RemovePredefinedFunctionReadOrWriteExtra, this) },
		{ "Synthesis Write function StatementList", std::bind(&LLParser::ExpandChildrenLastChildren, this) },
		{ "Synthesis Left curly bracket Write function Right curly bracket", std::bind(&LLParser::RemoveBrackets, this) },
		{ "Synthesis Else keyword StatementListBlock", std::bind(&LLParser::RemoveElseKeyword, this) },
		{ "Synthesis If keyword Left round bracket Identifier Right round bracket", std::bind(&LLParser::RemoveIfOrWhileStatementExtra, this) },
		{ "Synthesis IfCondition StatementListBlock StatementListBlock", std::bind(&LLParser::SynthesisIfOrWhileCondition, this) },
		{ "Synthesis IfCondition StatementListBlock", std::bind(&LLParser::SynthesisIfOrWhileConditionAndRemoveEmptyElse, this) },
		{ "Synthesis WhileCondition StatementListBlock", std::bind(&LLParser::SynthesisIfOrWhileCondition, this) },
		{ "Synthesis While keyword Left round bracket Identifier Right round bracket", std::bind(&LLParser::RemoveIfOrWhileStatementExtra, this) },
		{ "", std::bind(&LLParser::abc, this) },
	};

	std::unordered_set<std::string> IGNORED_ACTION_NAMES {
		"Synthesis Type Identifier",
		"Synthesis VariableDeclarationA Assignment Integer",
		"Synthesis VariableDeclarationA Assignment Float",
		"Synthesis VariableDeclarationA Assignment Identifier",
		"Synthesis VariableDeclarationA Assignment String",
		"Synthesis VariableDeclarationA Assignment Character",
		"Synthesis VariableDeclarationA Assignment String literal",
		"Synthesis VariableDeclarationA Assignment Character literal",
		"Synthesis VariableDeclarationA Assignment Boolean literal",
		"Synthesis VariableDeclarationA Assignment ArithmeticPlus",
		"Synthesis VariableDeclarationA Assignment ArithmeticMinus",
		"Synthesis VariableDeclarationA Assignment ArithmeticMultiply",
		"Synthesis VariableDeclarationA Assignment ArithmeticDivision",
		"Synthesis VariableDeclarationA Assignment ArithmeticIntegerDivision",
		"Synthesis VariableDeclarationA Assignment ArithmeticModule",
		"Synthesis Identifier Assignment Integer",
		"Synthesis Identifier Assignment Float",
		"Synthesis Identifier Assignment Identifier",
		"Synthesis Identifier Assignment String",
		"Synthesis Identifier Assignment Character",
		"Synthesis Identifier Assignment String literal",
		"Synthesis Identifier Assignment Character literal",
		"Synthesis Identifier Assignment Boolean literal",
		"Synthesis Identifier Assignment ArithmeticPlus",
		"Synthesis Identifier Assignment ArithmeticMinus",
		"Synthesis Identifier Assignment ArithmeticMultiply",
		"Synthesis Identifier Assignment ArithmeticDivision",
		"Synthesis Identifier Assignment ArithmeticIntegerDivision",
		"Synthesis Identifier Assignment ArithmeticModule",
		"Synthesis Assignment Identifier ArithmeticNegate",
		"Synthesis VariableDeclaration VariableDeclaration",
		"Synthesis VariableDeclaration If",
		"Synthesis VariableDeclaration While",
		"Synthesis VariableDeclaration Write function",
		"Synthesis Read function Write function",
		"Synthesis Assignment If",
		"Synthesis Write function Assignment"
	};

	std::unordered_map<std::string, std::unordered_set<std::string>> EXTRA_COMPATIBLE_TYPES = {
		{ TokenConstant::CoreType::Complex::STRING, { TokenConstant::Name::STRING_LITERAL }},
		{ TokenConstant::CoreType::CHARACTER, { TokenConstant::Name::CHARACTER_LITERAL }},
		{ TokenConstant::CoreType::BOOLEAN, { TokenConstant::Name::BOOLEAN_LITERAL }},
		{ TokenConstant::CoreType::Number::FLOAT, { TokenConstant::CoreType::Number::INTEGER }}
	};

	LLTableBuilder m_llTableBuilder;
	std::vector<AstNode *> m_ast;
	std::vector<std::unordered_map<std::string, unsigned int>> m_scopes {{ }};
	SymbolTable m_symbolTable;
	llvm::LLVMContext m_context;
	std::unique_ptr<llvm::Module> m_module;
	llvm::FunctionType * m_mainFunctionType;
	llvm::Function * m_mainFunction;
	llvm::BasicBlock * m_mainBlock;
	llvm::IRBuilder<> * m_builder;
	std::stack<llvm::BasicBlock*> m_preWhileBlocks;
	std::stack<llvm::BasicBlock*> m_whileBlocks;
	std::stack<llvm::BasicBlock*> m_blocksTrue;
	std::stack<llvm::BasicBlock*> m_blocksFalse;
	std::stack<llvm::BasicBlock*> m_previousBlocks;
};

#endif
