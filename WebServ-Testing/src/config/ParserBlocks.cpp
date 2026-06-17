#include "config/Parser.hpp"
#include <sstream>

void Parser::parseConfiguration()
{
	while (!isAtEnd())
	{
		const Token& tok = currentToken();

		if (tok.type == TOKEN_WORD && tok.value == "server")
			parseServer();
		else
			throw ParserException(formatError("Only 'server' blocks are allowed at top level", tok));
	}

	if (servers.empty())
		throw ParserException("Configuration must contain at least one server block");
}

void Parser::parseServer()
{
	expectWord("server", "server block");
	advance();

	expect(TOKEN_OPEN_BRACE, "server block");
	advance();

	ServerConfig server;
	while (!isAtEnd() && currentToken().type != TOKEN_CLOSE_BRACE)
		parseServerDirective(server);

	expect(TOKEN_CLOSE_BRACE, "server block");
	advance();

	servers.push_back(server);
}

void Parser::parseServerDirective(ServerConfig& server)
{
	const Token& tok = currentToken();
	if (tok.type != TOKEN_WORD)
		throw ParserException(formatError("Expected directive name", tok));

	const std::string& directive = tok.value;
	if (directive == "listen")
		parseListen(server);
	else if (directive == "host")
		parseHost(server);
	else if (directive == "server_name")
		parseServerName(server);
	else if (directive == "root")
		parseRoot(server);
	else if (directive == "index")
		parseIndex(server);
	else if (directive == "client_max_body_size")
		parseClientMaxBodySize(server);
	else if (directive == "error_page")
		parseErrorPage(server);
	else if (directive == "location")
		parseLocation(server);
	else
	{
		std::stringstream ss;
		ss << "Unknown server directive: " << directive;
		throw ParserException(formatError(ss.str(), tok));
	}
}

void Parser::parseLocation(ServerConfig& server)
{
	advance();
	std::string path = expectWord("location path");

	expect(TOKEN_OPEN_BRACE, "location block");
	advance();

	LocationConfig location(path);
	while (!isAtEnd() && currentToken().type != TOKEN_CLOSE_BRACE)
		parseLocationDirective(location);

	expect(TOKEN_CLOSE_BRACE, "location block");
	advance();

	server.addLocation(location);
}

void Parser::parseLocationDirective(LocationConfig& location)
{
	const Token& tok = currentToken();
	if (tok.type != TOKEN_WORD)
		throw ParserException(formatError("Expected directive name", tok));

	const std::string& directive = tok.value;
	if (directive == "methods")
		parseMethods(location);
	else if (directive == "root")
		parseLocationRoot(location);
	else if (directive == "index")
		parseLocationIndex(location);
	else if (directive == "autoindex")
		parseAutoindex(location);
	else if (directive == "upload_store")
		parseUploadStore(location);
	else if (directive == "cgi_extension")
		parseCgiExtension(location);
	else if (directive == "return")
		parseReturn(location);
	else
	{
		std::stringstream ss;
		ss << "Unknown location directive: " << directive;
		throw ParserException(formatError(ss.str(), tok));
	}
}
