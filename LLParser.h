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
	bool SynthesisType();
	bool SynthesisVariableDeclarationAAssignmentFloat();

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

		{ "Synthesis Plus Integer", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Float", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Identifier", std::bind(&LLParser::SynthesisPlus, this) },

		{ "Synthesis Minus Integer", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Float", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Identifier", std::bind(&LLParser::SynthesisMinus, this) },

		{ "Synthesis Multiply Integer", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Float", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Identifier", std::bind(&LLParser::SynthesisMultiply, this) },

		{ "Synthesis Integer division Integer", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Float", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Identifier", std::bind(&LLParser::SynthesisIntegerDivision, this) },

		{ "Synthesis Division Integer", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Float", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Identifier", std::bind(&LLParser::SynthesisDivision, this) },

		{ "Synthesis Modulus Integer", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Float", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Identifier", std::bind(&LLParser::SynthesisModulus, this) },

		{ "Synthesis Integer A", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Float A", std::bind(&LLParser::SynthesisType, this) },

		{ "Synthesis Integer B", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Float B", std::bind(&LLParser::SynthesisType, this) },

		{ "Synthesis Integer C", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Float C", std::bind(&LLParser::SynthesisType, this) },

		{ "Synthesis Integer D", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Float D", std::bind(&LLParser::SynthesisType, this) },

		{ "Synthesis Integer E", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Float E", std::bind(&LLParser::SynthesisType, this) },

		{ "Synthesis Integer F", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Float F", std::bind(&LLParser::SynthesisType, this) },

		{ "Synthesis Plus Integer B", std::bind(&LLParser::SynthesisPlus, this) },
	};

	std::unordered_set<std::string> IGNORED_ACTION_NAME {
		"Synthesis Assignment Semicolon",
		"Synthesis VariableDeclaration Semicolon",
		"Synthesis Type Identifier",
		"Synthesis Left curly bracket StatementList Right curly bracket",
		//"Synthesis VariableDeclarationA Assignment Integer",
		//"Synthesis VariableDeclarationA Assignment Float",
		//"Synthesis VariableDeclarationA Assignment String literal",
		//"Synthesis VariableDeclarationA Assignment Identifier",
		"Synthesis Identifier Assignment Integer",
		"Synthesis Left curly bracket Right curly bracket",
		"Synthesis SemicolonedVariableDeclaration SemicolonedVariableDeclaration",
		"Synthesis SemicolonedVariableDeclaration StatementList",
		"Synthesis SemicolonedVariableDeclaration SemicolonedAssignment",
		"Synthesis StatementListBlock StatementList",
		"Synthesis VariableDeclarationA Assignment String literal",
		"Synthesis VariableDeclarationA Assignment Integer",
		"Synthesis VariableDeclarationA Assignment Float"
	};

	std::unordered_map<std::string, std::unordered_set<std::string>> EXTRA_COMPATIBLE_TYPES = {
		{ TokenConstant::CoreType::Complex::STRING, { TokenConstant::Name::STRING_LITERAL }},
		{ TokenConstant::CoreType::Number::FLOAT, { TokenConstant::CoreType::Number::INTEGER }}
	};

	LLTableBuilder m_llTableBuilder;
	std::vector<AstNode *> m_ast;
	std::vector<std::unordered_map<std::string, unsigned int>> m_scopes {{ }};
	SymbolTable m_symbolTable;
};

#endif
