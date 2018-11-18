#ifndef LLPARSERLIBRARYEXAMPLE_CALCULATOR_H
#define LLPARSERLIBRARYEXAMPLE_CALCULATOR_H

#include <string>

class Calculator
{
public:
	static bool Add(
		std::string const & lhs,
		std::string const & rhs,
		std::string const & type,
		std::string & result,
		std::string & errorMessage
	);
	static bool Subtract(
		std::string const & lhs,
		std::string const & rhs,
		std::string const & type,
		std::string & result,
		std::string & errorMessage
	);
	static bool Multiply(
		std::string const & lhs,
		std::string const & rhs,
		std::string const & type,
		std::string & result,
		std::string & errorMessage
	);
	static bool IntegerDivision(
		std::string const & lhs,
		std::string const & rhs,
		std::string const & type,
		std::string & result,
		std::string & errorMessage
	);
	static bool Division(
		std::string const & lhs,
		std::string const & rhs,
		std::string const & type,
		std::string & result,
		std::string & errorMessage
	);
	static bool Modulus(
		std::string const & lhs,
		std::string const & rhs,
		std::string const & type,
		std::string & result,
		std::string & errorMessage
	);
};

#endif
