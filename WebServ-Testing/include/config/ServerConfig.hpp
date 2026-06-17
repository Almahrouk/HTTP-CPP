#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include "LocationConfig.hpp"

class ServerConfig
{
private:
	int port;
	std::string host;
	std::string serverName;
	std::string root;
	std::string index;
	size_t clientMaxBodySize;
	std::map<int, std::string> errorPages;
	std::vector<LocationConfig> locations;

public:
	ServerConfig();
	ServerConfig(const ServerConfig& other);
	ServerConfig& operator=(const ServerConfig& other);
	~ServerConfig();

	int getPort() const;
	const std::string& getHost() const;
	const std::string& getServerName() const;
	const std::string& getRoot() const;
	const std::string& getIndex() const;
	size_t getClientMaxBodySize() const;
	const std::map<int, std::string>& getErrorPages() const;
	const std::vector<LocationConfig>& getLocations() const;

	void setPort(int port);
	void setHost(const std::string& host);
	void setServerName(const std::string& serverName);
	void setRoot(const std::string& root);
	void setIndex(const std::string& index);
	void setClientMaxBodySize(size_t size);
	void addErrorPage(int code, const std::string& path);
	void addLocation(const LocationConfig& location);

	std::string getErrorPage(int code) const;
	const LocationConfig* findLocation(const std::string& path) const;
};

#endif
