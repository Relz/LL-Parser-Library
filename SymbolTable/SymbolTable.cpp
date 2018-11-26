#include "SymbolTable.h"
#include "SymbolTableRow/ArrayInformation/ArrayInformation.h"
#include <string>
#include <algorithm>

unsigned int SymbolTable::CreateRow(
	std::string const & type, std::string const & name, std::vector<unsigned int> const & dimensions)
{
	SymbolTableRow symbolTableRow;
	symbolTableRow.type = type;
	symbolTableRow.name = name;
	if (!dimensions.empty())
	{
		ArrayInformation * arrayInformation = new ArrayInformation();
		arrayInformation->dimensions = dimensions;
		symbolTableRow.arrayInformation = arrayInformation;
	}
	m_table.emplace_back(symbolTableRow);

	return m_table.size() - 1;
}

bool SymbolTable::RemoveRow(unsigned int rowIndex)
{
	bool result = rowIndex < m_table.size();
	if (result)
	{
		m_table[rowIndex].name = "";
		m_table[rowIndex].type = "";
		m_table[rowIndex].arrayInformation = nullptr;
	}
	return result;
}

bool SymbolTable::GetSymbolTableRowByRowIndex(unsigned int rowIndex, SymbolTableRow & result) const
{
	if (rowIndex < m_table.size())
	{
		result = m_table[rowIndex];

		return true;
	}
	return false;
}
