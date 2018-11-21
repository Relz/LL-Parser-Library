#ifndef LLPARSERLIBRARYEXAMPLE_SYMBOLTABLE_H
#define LLPARSERLIBRARYEXAMPLE_SYMBOLTABLE_H

#include <vector>
#include <string>
#include "SymbolTableRow/SymbolTableRow.h"

class SymbolTable
{
public:
	unsigned int CreateRow(std::string const & type, std::string const & name, std::vector<unsigned int> const & dimensions);
	bool RemoveRow(unsigned int rowIndex);
	bool GetSymbolTableRowByRowIndex(unsigned int rowIndex, SymbolTableRow & result);

private:
	std::vector<SymbolTableRow> m_table;
};

#endif
