#ifndef LLPARSERLIBRARYEXAMPLE_SYMBOLTABLEROW_H
#define LLPARSERLIBRARYEXAMPLE_SYMBOLTABLEROW_H

#include "ArrayInformation/ArrayInformation.h"
#include <string>
#include <llvm/IR/Instructions.h>

class SymbolTableRow
{
public:
	std::string type;
	std::string name;
	llvm::AllocaInst * llvmAllocaInst;
	ArrayInformation * arrayInformation = nullptr;
};

#endif
