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
		m_table.erase(m_table.begin() + rowIndex, m_table.begin() + rowIndex + 1);
	}
	return result;
}

bool SymbolTable::GetSymbolTableRowByName(std::string const & name, SymbolTableRow & result)
{
	auto resultIt = std::find_if(m_table.begin(), m_table.end(), [&name](SymbolTableRow const & symbolTableRow) {
		return symbolTableRow.name == name;
	});
	if (resultIt == m_table.end())
	{
		return false;
	}
	else
	{
		result = *resultIt;

		return true;
	}
}
