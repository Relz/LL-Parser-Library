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

	bool CreateScopeAction();
	bool DestroyScopeAction();
	void computeDimensions(std::vector<unsigned int> & dimensions);
	bool AddVariableToScope();
	bool CheckIdentifierForAlreadyExisting();
	bool CheckIdentifierForExisting();
	bool Synthesis();
	bool SynthesisPlus();
	bool SynthesisMinus();
	bool SynthesisMultiply();
	bool SynthesisIntegerDivision();
	bool SynthesisDivision();
	bool SynthesisModulus();
	bool CheckVariableTypeWithAssignmentRightHandTypeForEquality();
	bool CheckIdentifierTypeWithAssignmentRightHandTypeForEquality();
	bool SynthesisType();

	void PrintColoredMessage(std::string const & message, std::string const & colorCode) const;
	void PrintWarningMessage(std::string const & message) const;
	void PrintErrorMessage(std::string const & message) const;

	std::unordered_map<std::string, std::function<bool()>> const ACTION_NAME_TO_ACTION_MAP {
		{ "Create scope", std::bind(&LLParser::CreateScopeAction, this) },
		{ "Destroy scope", std::bind(&LLParser::DestroyScopeAction, this) },
		{ "Add variable to scope", std::bind(&LLParser::AddVariableToScope, this) },
		{ "Check identifier for already existing", std::bind(&LLParser::CheckIdentifierForAlreadyExisting, this) },
		{ "Check identifier for existing", std::bind(&LLParser::CheckIdentifierForExisting, this) },
		{ "Synthesis", std::bind(&LLParser::Synthesis, this) },
		{ "Check variable type with AssignmentRightHand type for equality", std::bind(&LLParser::CheckVariableTypeWithAssignmentRightHandTypeForEquality, this) },
		{ "Check identifier type with AssignmentRightHand type for equality", std::bind(&LLParser::CheckIdentifierTypeWithAssignmentRightHandTypeForEquality, this) },

		{ "Synthesis Plus Integer", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Integer B", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Float", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Float B", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Identifier", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Identifier B", std::bind(&LLParser::SynthesisPlus, this) },
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
	};

	std::unordered_set<std::string> IGNORED_ACTION_NAME {
		"Synthesis Assignment Semicolon",
		"Synthesis VariableDeclaration Semicolon",
		"Synthesis Type Identifier",
		"Synthesis Left curly bracket StatementList Right curly bracket",
		"Synthesis Identifier Assignment Integer",
		"Synthesis Left curly bracket Right curly bracket",
		"Synthesis SemicolonedVariableDeclaration SemicolonedVariableDeclaration",
		"Synthesis SemicolonedVariableDeclaration StatementList",
		"Synthesis SemicolonedVariableDeclaration SemicolonedAssignment",
		"Synthesis StatementListBlock StatementList",
		"Synthesis VariableDeclarationA Assignment String literal",
		"Synthesis VariableDeclarationA Assignment Integer",
		"Synthesis VariableDeclarationA Assignment Float",
		"Synthesis VariableDeclarationA Assignment Character literal",
		"Synthesis VariableDeclarationA Assignment ArithmeticPlus",
		"Synthesis VariableDeclarationA Assignment ArithmeticMinus",
		"Synthesis VariableDeclarationA Assignment ArithmeticMultiply",
		"Synthesis VariableDeclarationA Assignment ArithmeticDivision",
		"Synthesis VariableDeclarationA Assignment ArithmeticIntegerDivision",
		"Synthesis VariableDeclarationA Assignment ArithmeticModule",
		"Synthesis Identifier Assignment ArithmeticPlus",
		"Synthesis Identifier Assignment ArithmeticMinus",
		"Synthesis Identifier Assignment ArithmeticMultiply",
		"Synthesis SemicolonedAssignment SemicolonedAssignment",
		"Synthesis SemicolonedAssignment StatementList",
	};

	std::unordered_map<std::string, std::unordered_set<std::string>> EXTRA_COMPATIBLE_TYPES = {
		{ TokenConstant::CoreType::Complex::STRING, { TokenConstant::Name::STRING_LITERAL }},
		{ TokenConstant::CoreType::CHARACTER, { TokenConstant::Name::CHARACTER_LITERAL }},
		{ TokenConstant::CoreType::Number::FLOAT, { TokenConstant::CoreType::Number::INTEGER }}
	};

	LLTableBuilder m_llTableBuilder;
	std::vector<AstNode *> m_ast;
	std::vector<std::unordered_map<std::string, unsigned int>> m_scopes {{ }};
	SymbolTable m_symbolTable;
};

#endif
