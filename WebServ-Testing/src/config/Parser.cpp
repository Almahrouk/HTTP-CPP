#include "config/Parser.hpp"
#include <sstream>

Parser::Parser(const std::vector<Token>& tokens)
	: token(tokens),
	  index(0),
	  servers()
{
}

Parser::~Parser()
{
}

void Parser::parse()
{
	parseConfiguration();
}

const std::vector<ServerConfig>& Parser::getServers() const
{
	return servers;
}

const Token& Parser::currentToken() const
{
	if (index >= token.size())
		return token[token.size() - 1];
	return token[index];
}

const Token& Parser::peekToken(size_t offset) const
{
	size_t peekIndex = index + offset;
	if (peekIndex >= token.size())
		return token[token.size() - 1];
	return token[peekIndex];
}

bool Parser::isAtEnd() const
{
	return currentToken().type == TOKEN_END;
}

void Parser::advance()
{
	if (index < token.size())
		++index;
}

void Parser::expect(TokenType type, const std::string& context)
{
	const Token& tok = currentToken();
	if (tok.type != type)
	{
		std::stringstream ss;
		ss << "Expected ";

		switch (type)
		{
			case TOKEN_WORD: ss << "word"; break;
			case TOKEN_SEMICOLON: ss << "semicolon"; break;
			case TOKEN_OPEN_BRACE: ss << "open brace"; break;
			case TOKEN_CLOSE_BRACE: ss << "close brace"; break;
			case TOKEN_END: ss << "end of file"; break;
		}

		ss << " in " << context;
		throw ParserException(formatError(ss.str(), tok));
	}
}

void Parser::expectWord(const std::string& expected, const std::string& context)
{
	expect(TOKEN_WORD, context);
	const Token& tok = currentToken();

	if (tok.value != expected)
	{
		std::stringstream ss;
		ss << "Expected '" << expected << "' in " << context << ", got '" << tok.value << "'";
		throw ParserException(formatError(ss.str(), tok));
	}
}

std::string Parser::expectWord(const std::string& context)
{
	expect(TOKEN_WORD, context);
	std::string value = currentToken().value;
	advance();
	return value;
}

std::string Parser::formatError(const std::string& message, const Token& token) const
{
	std::stringstream ss;
	ss << message << " at line " << token.line << ", column " << token.column;
	return ss.str();
}
