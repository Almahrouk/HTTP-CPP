#include "../../include/config/ServerConfig.hpp"

ServerConfig::ServerConfig()
	: port(80),
	  host("0.0.0.0"),
	  serverName(""),
	  root(""),
	  index("index.html"),
	  clientMaxBodySize(1048576),
	  errorPages(),
	  locations()
{
}

ServerConfig::ServerConfig(const ServerConfig& other)
	: port(other.port),
	  host(other.host),
	  serverName(other.serverName),
	  root(other.root),
	  index(other.index),
	  clientMaxBodySize(other.clientMaxBodySize),
	  errorPages(other.errorPages),
	  locations(other.locations)
{
}

ServerConfig& ServerConfig::operator=(const ServerConfig& other)
{
	if (this != &other)
	{
		port = other.port;
		host = other.host;
		serverName = other.serverName;
		root = other.root;
		index = other.index;
		clientMaxBodySize = other.clientMaxBodySize;
		errorPages = other.errorPages;
		locations = other.locations;
	}
	return *this;
}

ServerConfig::~ServerConfig()
{
}

int ServerConfig::getPort() const { return port; }
const std::string& ServerConfig::getHost() const { return host; }
const std::string& ServerConfig::getServerName() const { return serverName; }
const std::string& ServerConfig::getRoot() const { return root; }
const std::string& ServerConfig::getIndex() const { return index; }
size_t ServerConfig::getClientMaxBodySize() const { return clientMaxBodySize; }
const std::map<int, std::string>& ServerConfig::getErrorPages() const { return errorPages; }
const std::vector<LocationConfig>& ServerConfig::getLocations() const { return locations; }

void ServerConfig::setPort(int port) { this->port = port; }
void ServerConfig::setHost(const std::string& host) { this->host = host; }
void ServerConfig::setServerName(const std::string& serverName) { this->serverName = serverName; }
void ServerConfig::setRoot(const std::string& root) { this->root = root; }
void ServerConfig::setIndex(const std::string& index) { this->index = index; }
void ServerConfig::setClientMaxBodySize(size_t size) { this->clientMaxBodySize = size; }
void ServerConfig::addErrorPage(int code, const std::string& path) { errorPages[code] = path; }
void ServerConfig::addLocation(const LocationConfig& location) { locations.push_back(location); }

std::string ServerConfig::getErrorPage(int code) const
{
	std::map<int, std::string>::const_iterator it = errorPages.find(code);
	if (it != errorPages.end())
		return it->second;
	return "";
}

const LocationConfig* ServerConfig::findLocation(const std::string& path) const
{
	const LocationConfig* bestMatch = 0;
	size_t bestMatchLength = 0;

	for (size_t i = 0; i < locations.size(); ++i)
	{
		const std::string& locPath = locations[i].getPath();
		size_t locLen = locPath.length();

		if (path.compare(0, locLen, locPath) == 0)
		{
			if (locLen > bestMatchLength)
			{
				bestMatch = &locations[i];
				bestMatchLength = locLen;
			}
		}
	}

	return bestMatch;
}