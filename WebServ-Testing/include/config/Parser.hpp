#ifndef PARSER_HPP
#define PARSER_HPP

#include <vector>
#include <string>
#include "Token.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include "ConfigException.hpp"

class Parser
{
private:
	std::vector<Token> token;
	size_t index;
	std::vector<ServerConfig> servers;

	const Token& currentToken() const;
	const Token& peekToken(size_t offset = 1) const;
	bool isAtEnd() const;
	void advance();
	
	void expect(TokenType type, const std::string& context);
	void expectWord(const std::string& expected, const std::string& context);
	std::string expectWord(const std::string& context);
	
	void parseConfiguration();
	void parseServer();
	void parseServerDirective(ServerConfig& server);
	void parseLocation(ServerConfig& server);
	void parseLocationDirective(LocationConfig& location);
	
	void parseListen(ServerConfig& server);
	void parseHost(ServerConfig& server);
	void parseServerName(ServerConfig& server);
	void parseRoot(ServerConfig& server);
	void parseIndex(ServerConfig& server);
	void parseClientMaxBodySize(ServerConfig& server);
	void parseErrorPage(ServerConfig& server);
	
	void parseLocationRoot(LocationConfig& location);
	void parseLocationIndex(LocationConfig& location);
	void parseMethods(LocationConfig& location);
	void parseAutoindex(LocationConfig& location);
	void parseUploadStore(LocationConfig& location);
	void parseCgiExtension(LocationConfig& location);
	void parseReturn(LocationConfig& location);
	
	size_t parseBodySize(const std::string& value);
	int parseNumber(const std::string& value);
	std::string formatError(const std::string& message, const Token& token) const;

public:
	explicit Parser(const std::vector<Token>& tokens);
	~Parser();

	void parse();
	const std::vector<ServerConfig>& getServers() const;
};

#endif
