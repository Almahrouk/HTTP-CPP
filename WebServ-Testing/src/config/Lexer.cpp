#include "config/Lexer.hpp"
#include <iostream>
#include <sstream>

Lexer::Lexer(const std::string& filename)
	: filename(filename),
	  content(""),
	  token(),
	  pos(0),
	  line(1),
	  column(1)
{
	signal(SIGPIPE, SIG_IGN);
	readFile();
	removeComments();
	tokenize();
	addToken(TOKEN_END, "");
}

Lexer::~Lexer()
{
}

void Lexer::readFile()
{
	std::ifstream file(filename.c_str());
	if (!file.is_open())
		throw LexerException("Cannot open file: " + filename);

	std::stringstream buffer;
	buffer << file.rdbuf();
	content = buffer.str();
	file.close();

	if (content.empty())
		throw LexerException("File is empty: " + filename);
}

void Lexer::removeComments()
{
	std::string result;
	result.reserve(content.size());

	for (size_t i = 0; i < content.size(); ++i)
	{
		if (content[i] == '#')
		{
			while (i < content.size() && content[i] != '\n')
				++i;
			if (i < content.size())
				result += content[i];
		}
		else
		{
			result += content[i];
		}
	}

	content = result;
}

void Lexer::tokenize()
{
	pos = 0;
	line = 1;
	column = 1;

	while (pos < content.size())
	{
		skipWhitespace();
		
		if (pos >= content.size())
			break;

		char c = currentChar();

		if (c == ';')
		{
			addToken(TOKEN_SEMICOLON, ";");
			advance();
		}
		else if (c == '{')
		{
			addToken(TOKEN_OPEN_BRACE, "{");
			advance();
		}
		else if (c == '}')
		{
			addToken(TOKEN_CLOSE_BRACE, "}");
			advance();
		}
		else if (isWordChar(c))
		{
			token.push_back(readWord());
		}
		else
		{
			std::stringstream ss;
			ss << "Unexpected character '" << c << "' at line " << line << ", column " << column;
			throw LexerException(ss.str());
		}
	}
}

bool Lexer::isWhitespace(char c) const
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool Lexer::isWordChar(char c) const
{
	return (c >= 'a' && c <= 'z') ||
	       (c >= 'A' && c <= 'Z') ||
	       (c >= '0' && c <= '9') ||
	       c == '_' || c == '-' || c == '.' || c == '/' ||
	       c == ':' || c == '*' || c == '?' || c == '~' || c == '$' || c == '@' || c == '=' ;
}

void Lexer::skipWhitespace()
{
	while (pos < content.size() && isWhitespace(currentChar()))
	{
		if (currentChar() == '\n')
		{
			++line;
			column = 0;
		}
		advance();
	}
}

Token Lexer::readWord()
{
	size_t startLine = line;
	size_t startColumn = column;
	std::string word;

	while (pos < content.size() && isWordChar(currentChar()))
	{
		word += currentChar();
		advance();
	}

	return Token(TOKEN_WORD, word, startLine, startColumn);
}

void Lexer::addToken(TokenType type, const std::string& value)
{
	token.push_back(Token(type, value, line, column));
}

char Lexer::currentChar() const
{
	if (pos >= content.size())
		return '\0';
	return content[pos];
}

char Lexer::peekChar(size_t offset) const
{
	if (pos + offset >= content.size())
		return '\0';
	return content[pos + offset];
}

void Lexer::advance()
{
	if (pos < content.size())
	{
		++pos;
		++column;
	}
}

const std::vector<Token>& Lexer::getTokens() const
{
	return token;
}

