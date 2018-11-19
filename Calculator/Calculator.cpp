#include "Calculator.h"
#include "../LLTableBuilderLibrary/TokenLibrary/TokenExtensions/TokenExtensions.h"

bool Calculator::Add(
	std::string const & lhs,
	std::string const & rhs,
	std::string const & type,
	std::string & result,
	std::string & errorMessage
)
{
	Token token;
	if (!TokenExtensions::CreateFromString(type, token))
	{
		errorMessage = "Unknown type \"" + type + "\"" + "\n";

		return false;
	}
	switch (token)
	{
		case Token::INTEGER:
			result = std::to_string(stoi(lhs) + stoi(rhs));
			break;
		case Token::FLOAT:
			result = std::to_string(stof(lhs) + stof(rhs));
			break;
		default:
			errorMessage = "Unsupported type for adding: \"" + type + "\"" + "\n";

			return false;
	}
	return true;
}

bool Calculator::Subtract(
	std::string const & lhs,
	std::string const & rhs,
	std::string const & type,
	std::string & result,
	std::string & errorMessage
)
{
	Token token;
	if (!TokenExtensions::CreateFromString(type, token))
	{
		errorMessage = "Unknown type \"" + type + "\"" + "\n";

		return false;
	}
	switch (token)
	{
		case Token::INTEGER:
			result = std::to_string(stoi(lhs) - stoi(rhs));
			break;
		case Token::FLOAT:
			result = std::to_string(stof(lhs) - stof(rhs));
			break;
		default:
			errorMessage = "Unsupported type for subtracting: \"" + type + "\"" + "\n";

			return false;
	}
	return true;
}

bool Calculator::Multiply(
	std::string const & lhs,
	std::string const & rhs,
	std::string const & type,
	std::string & result,
	std::string & errorMessage
)
{
	Token token;
	if (!TokenExtensions::CreateFromString(type, token))
	{
		errorMessage = "Unknown type \"" + type + "\"" + "\n";
		return false;
	}
	switch (token)
	{
		case Token::INTEGER:
			result = std::to_string(stoi(lhs) * stoi(rhs));
			break;
		case Token::FLOAT:
			result = std::to_string(stof(lhs) * stof(rhs));
			break;
		default:
			errorMessage = "Unsupported type for multiplying: \"" + type + "\"" + "\n";

			return false;
	}
	return true;
}

bool Calculator::IntegerDivision(
	std::string const & lhs,
	std::string const & rhs,
	std::string const & type,
	std::string & result,
	std::string & errorMessage
)
{
	Token token;
	if (!TokenExtensions::CreateFromString(type, token))
	{
		errorMessage = "Unknown type \"" + type + "\"" + "\n";

		return false;
	}
	int rhsInteger = stoi(rhs);
	if (rhsInteger == 0)
	{
		errorMessage = std::string("Cannot divide by zero") + "\n";

		return false;
	}
	switch (token)
	{
		case Token::INTEGER:
		case Token::FLOAT:
			result = std::to_string(stoi(lhs) / rhsInteger);
			break;
		default:
			errorMessage = "Unsupported type for integer dividing: \"" + type + "\"" + "\n";

			return false;
	}
	return true;
}

bool Calculator::Division(
	std::string const & lhs,
	std::string const & rhs,
	std::string const & type,
	std::string & result,
	std::string & errorMessage
)
{
	Token token;
	if (!TokenExtensions::CreateFromString(type, token))
	{
		errorMessage = "Unknown type \"" + type + "\"" + "\n";

		return false;
	}
	switch (token)
	{
		case Token::INTEGER:
		case Token::FLOAT:
			result = std::to_string(stof(lhs) / stof(rhs));
			break;
		default:
			errorMessage = "Unsupported type for dividing: \"" + type + "\"" + "\n";

			return false;
	}
	return true;
}

bool Calculator::Modulus(
	std::string const & lhs,
	std::string const & rhs,
	std::string const & type,
	std::string & result,
	std::string & errorMessage
)
{
	Token token;
	if (!TokenExtensions::CreateFromString(type, token))
	{
		errorMessage = "Unknown type \"" + type + "\"" + "\n";

		return false;
	}
	switch (token)
	{
		case Token::INTEGER:
		case Token::FLOAT:
			result = std::to_string(stoi(lhs) % stoi(rhs));
			break;
		default:
			errorMessage = "Unsupported type for moduling: \"" + type + "\"" + "\n";

			return false;
	}
	return true;
}
