#include "config/Parser.hpp"
#include <sstream>
#include <cstdlib>
#include <cctype>

size_t Parser::parseBodySize(const std::string& value)
{
	if (value.empty())
		throw ParserException("Empty body size value");

	std::string numPart;
	char suffix = '\0';

	for (size_t i = 0; i < value.length(); ++i)
	{
		char c = value[i];
		if (std::isdigit(c))
			numPart += c;
		else if (i == value.length() - 1 && (c == 'K' || c == 'M' || c == 'G'))
			suffix = c;
		else
		{
			std::stringstream ss;
			ss << "Invalid body size format: " << value;
			throw ParserException(ss.str());
		}
	}

	if (numPart.empty())
		throw ParserException("Body size must contain a number");

	size_t num = static_cast<size_t>(std::atol(numPart.c_str()));
	if (suffix == 'K')
		return num * 1024;
	if (suffix == 'M')
		return num * 1024 * 1024;
	if (suffix == 'G')
		return num * 1024 * 1024 * 1024;
	return num;
}

int Parser::parseNumber(const std::string& value)
{
	if (value.empty())
		throw ParserException("Empty number value");

	for (size_t i = 0; i < value.length(); ++i)
	{
		if (!std::isdigit(value[i]))
		{
			std::stringstream ss;
			ss << "Invalid number format: " << value;
			throw ParserException(ss.str());
		}
	}

	return std::atoi(value.c_str());
}
