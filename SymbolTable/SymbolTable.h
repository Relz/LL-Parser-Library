#ifndef LLPARSERLIBRARYEXAMPLE_SYMBOLTABLE_H
#define LLPARSERLIBRARYEXAMPLE_SYMBOLTABLE_H

#include <vector>
#include <string>
#include "SymbolTableRow/SymbolTableRow.h"
#include <llvm/IR/Instructions.h>

class SymbolTable
{
public:
	unsigned int CreateRow(
		std::string const & type,
		std::string const & name,
		llvm::AllocaInst * llvmAllocaInst,
		std::vector<unsigned int> const & dimensions
	);
	bool RemoveRow(unsigned int rowIndex);
	bool GetSymbolTableRowByRowIndex(unsigned int rowIndex, SymbolTableRow & result) const;

private:
	std::vector<SymbolTableRow> m_table;
};

#endif
