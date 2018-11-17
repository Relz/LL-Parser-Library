#ifndef LLPARSERLIBRARY_LIBRARY_H
#define LLPARSERLIBRARY_LIBRARY_H

#include "LLTableBuilderLibrary/LLTableBuilder.h"
#include "AstNode/AstNode.h"
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
	) const;

private:
	AstNode * CreateAstNode(
		std::string const & ruleName, unsigned int tokenCount, std::stack<AstNode *> & astStack
	) const;

	bool TryToCreateAstNode(
		std::string const & actionName, std::stack<AstNode *> & astStack
	) const;

	static bool ParseCreateAstNodeAction(std::string const & actionName, std::string & ruleName, unsigned int & tokenCount);
	void ResolveActionName(std::string const & actionName) const;
	void ResolveAstActionName(std::string const & actionName, std::stack<AstNode *> & astStack) const;

	void ProgramAction();
	void VariableDeclarationAction();
	void AssignmentAction();

	std::unordered_map<std::string, std::function<void()>> const ACTION_NAME_TO_ACTION_MAP {
			{ "ProgramAction", std::bind(&LLParser::ProgramAction, this) },
			{ "VariableDeclarationAction", std::bind(&LLParser::VariableDeclarationAction, this) },
			{ "AssignmentAction", std::bind(&LLParser::AssignmentAction, this) },
	};

	LLTableBuilder m_llTableBuilder;

};

#endif
