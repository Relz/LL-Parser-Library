#ifndef LLPARSERLIBRARYEXAMPLE_SYMBOLTABLEROW_H
#define LLPARSERLIBRARYEXAMPLE_SYMBOLTABLEROW_H

#include "ArrayInformation/ArrayInformation.h"
#include <string>

class SymbolTableRow
{
public:
	std::string type;
	std::string name;
	ArrayInformation * arrayInformation = nullptr;
};

#endif
