#include "config/Parser.hpp"
#include <sstream>

void Parser::parseLocationRoot(LocationConfig& location)
{
	advance();
	std::string root = expectWord("root directive");

	expect(TOKEN_SEMICOLON, "root directive");
	advance();

	location.setRoot(root);
}

void Parser::parseLocationIndex(LocationConfig& location)
{
	advance();
	std::string index = expectWord("index directive");

	expect(TOKEN_SEMICOLON, "index directive");
	advance();

	location.setIndex(index);
}

void Parser::parseMethods(LocationConfig& location)
{
	advance();
	while (!isAtEnd() && currentToken().type == TOKEN_WORD)
	{
		location.addMethod(currentToken().value);
		advance();
	}
	if(location.getMethods().empty())
	{
		std::stringstream ss;
		ss << "Methods cannot be Empty";
		throw ParserException(ss.str());
	}
	expect(TOKEN_SEMICOLON, "methods directive");
	advance();
}

void Parser::parseAutoindex(LocationConfig& location)
{
	advance();
	std::string value = expectWord("autoindex directive");

	expect(TOKEN_SEMICOLON, "autoindex directive");
	advance();

	if (value == "on")
		location.setAutoindex(true);
	else if (value == "off")
		location.setAutoindex(false);
	else
	{
		std::stringstream ss;
		ss << "Invalid autoindex value: " << value << " (expected 'on' or 'off')";
		throw ParserException(ss.str());
	}
}

void Parser::parseUploadStore(LocationConfig& location)
{
	advance();
	std::string path = expectWord("upload_store directive");

	expect(TOKEN_SEMICOLON, "upload_store directive");
	advance();

	location.setUploadStore(path);
}

void Parser::parseCgiExtension(LocationConfig& location)
{
	advance();
	std::string ext = expectWord("cgi_extension directive");
	std::string interpreter = expectWord("cgi_extension directive");

	expect(TOKEN_SEMICOLON, "cgi_extension directive");
	advance();

	location.addCgiExtension(ext, interpreter);
}

void Parser::parseReturn(LocationConfig& location)
{
	advance();
	std::string codeStr = expectWord("return directive");
	int code = parseNumber(codeStr);
	std::string url = expectWord("return directive");

	expect(TOKEN_SEMICOLON, "return directive");
	advance();

	location.setRedirect(code, url);
}
