#ifndef CONFIGEXCEPTION_HPP
#define CONFIGEXCEPTION_HPP

#include <exception>
#include <string>

class ConfigException : public std::exception
{
private:
	std::string message;

public:
	explicit ConfigException(const std::string& message) : message(message) {}
	virtual ~ConfigException() throw() {}

	virtual const char* what() const throw()
	{
		return message.c_str();
	}
};

class LexerException : public ConfigException
{
public:
	explicit LexerException(const std::string& message) 
		: ConfigException("Lexer Error: " + message) {}
	virtual ~LexerException() throw() {}
};

class ParserException : public ConfigException
{
public:
	explicit ParserException(const std::string& message) 
		: ConfigException("Parser Error: " + message) {}
	virtual ~ParserException() throw() {}
};

class ValidatorException : public ConfigException
{
public:
	explicit ValidatorException(const std::string& message) 
		: ConfigException("Validator Error: " + message) {}
	virtual ~ValidatorException() throw() {}
};

#endif
