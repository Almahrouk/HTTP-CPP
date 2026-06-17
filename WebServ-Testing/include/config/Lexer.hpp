#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <vector>
#include <fstream>
#include <signal.h>
#include "Token.hpp"
#include "ConfigException.hpp"

class Lexer
{
private:
	std::string filename;
	std::string content;
	std::vector<Token> token;
	size_t pos;
	size_t line;
	size_t column;

	void readFile();
	void removeComments();
	void tokenize();
	
	bool isWhitespace(char c) const;
	bool isWordChar(char c) const;
	void skipWhitespace();
	Token readWord();
	void addToken(TokenType type, const std::string& value);
	char currentChar() const;
	char peekChar(size_t offset = 1) const;
	void advance();

public:
	explicit Lexer(const std::string& filename);
	~Lexer();

	const std::vector<Token>& getTokens() const;
};

#endif
