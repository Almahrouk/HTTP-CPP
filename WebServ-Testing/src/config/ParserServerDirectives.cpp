#include "config/Parser.hpp"

void Parser::parseListen(ServerConfig& server)
{
	advance();
	std::string portStr = expectWord("listen directive");
	int port = parseNumber(portStr);

	expect(TOKEN_SEMICOLON, "listen directive");
	advance();

	server.setPort(port);
}

void Parser::parseHost(ServerConfig& server)
{
	advance();
	std::string host = expectWord("host directive");

	expect(TOKEN_SEMICOLON, "host directive");
	advance();

	server.setHost(host);
}

void Parser::parseServerName(ServerConfig& server)
{
	advance();
	std::string name = expectWord("server_name directive");

	expect(TOKEN_SEMICOLON, "server_name directive");
	advance();

	server.setServerName(name);
}

void Parser::parseRoot(ServerConfig& server)
{
	advance();
	std::string root = expectWord("root directive");

	expect(TOKEN_SEMICOLON, "root directive");
	advance();

	server.setRoot(root);
}

void Parser::parseIndex(ServerConfig& server)
{
	advance();
	std::string index = expectWord("index directive");

	expect(TOKEN_SEMICOLON, "index directive");
	advance();

	server.setIndex(index);
}

void Parser::parseClientMaxBodySize(ServerConfig& server)
{
	advance();
	std::string sizeStr = expectWord("client_max_body_size directive");
	size_t size = parseBodySize(sizeStr);

	expect(TOKEN_SEMICOLON, "client_max_body_size directive");
	advance();

	server.setClientMaxBodySize(size);
}

void Parser::parseErrorPage(ServerConfig& server)
{
	advance();
	std::string codeStr = expectWord("error_page directive");
	int code = parseNumber(codeStr);
	std::string path = expectWord("error_page directive");

	expect(TOKEN_SEMICOLON, "error_page directive");
	advance();

	server.addErrorPage(code, path);
}
