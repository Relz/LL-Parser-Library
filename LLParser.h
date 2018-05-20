#ifndef LLPARSERLIBRARY_LIBRARY_H
#define LLPARSERLIBRARY_LIBRARY_H

#include <string>
#include "LLTableBuilderLibrary/LLTableBuilder.h"

class TokenInformation;

class LLParser
{
public:
	explicit LLParser(std::string const & ruleFileName);

	bool IsValid(
		std::string const & inputFileName,
		std::vector<TokenInformation> & tokenInformations,
		size_t & failIndex,
		std::unordered_set<Token> & expectedWords) const;

private:
	static std::string const END_OF_LINE_STRING;

	LLTableBuilder m_llTableBuilder;
};

#endif
