#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>

enum TokenType
{
	TOKEN_WORD,
	TOKEN_SEMICOLON,
	TOKEN_OPEN_BRACE,
	TOKEN_CLOSE_BRACE,
	TOKEN_END
};

struct Token
{
	TokenType type;
	std::string value;
	size_t line;
	size_t column;

	Token() : type(TOKEN_END), value(""), line(0), column(0) {}
	
	Token(TokenType t, const std::string& v, size_t l, size_t c)
		: type(t), value(v), line(l), column(c) {}
};

#endif
