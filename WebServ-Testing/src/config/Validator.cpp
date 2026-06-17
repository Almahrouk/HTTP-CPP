#include "config/Validator.hpp"
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

Validator::Validator(const std::vector<ServerConfig>& servers)
	: servers(servers)
{
}

Validator::~Validator()
{
}

void Validator::validate()
{
	if (servers.empty())
		throw ValidatorException("No servers to validate");

	validateServers();
	checkDuplicatePorts();
}

void Validator::validateServers()
{
	for (size_t i = 0; i < servers.size(); ++i)
	{
		validateServer(servers[i]);
	}
}

void Validator::validateServer(const ServerConfig& server)
{
	int port = server.getPort();
	if (!isValidPort(port))
	{
		std::stringstream ss;
		ss << "Invalid port number: " << port << " (must be between 1 and 65535)";
		throw ValidatorException(ss.str());
	}

	if (server.getRoot().empty())
		throw ValidatorException("Server root cannot be empty");

	if (server.getClientMaxBodySize() == 0)
		throw ValidatorException("client_max_body_size must be greater than 0");

	if (server.getHost().empty())
		throw ValidatorException("Server host cannot be empty");

	const std::map<int, std::string>& errorPages = server.getErrorPages();
	for (std::map<int, std::string>::const_iterator it = errorPages.begin(); it != errorPages.end(); ++it)
	{
		if (it->first < 100 || it->first > 599)
		{
			std::stringstream ss;
			ss << "Invalid error page code: " << it->first;
			throw ValidatorException(ss.str());
		}
		if (it->second.empty())
		{
			std::stringstream ss;
			ss << "Error page path cannot be empty for code " << it->first;
			throw ValidatorException(ss.str());
		}
	}

	const std::vector<LocationConfig>& locations = server.getLocations();
	for (size_t i = 0; i < locations.size(); ++i)
	{
		validateLocation(locations[i]);
	}
}

void Validator::validateLocation(const LocationConfig& location)
{
	if (location.getPath().empty())
		throw ValidatorException("Location path cannot be empty");

	const std::vector<std::string>& methods = location.getMethods();
		
	for (size_t i = 0; i < methods.size(); ++i)
	{
		if (!isValidMethod(methods[i]))
		{
			std::stringstream ss;
			ss << "Invalid HTTP method: " << methods[i] << " (only GET, POST, DELETE allowed)";
			throw ValidatorException(ss.str());
		}
	}

	if (location.hasRedirect())
	{
		int code = location.getRedirectCode();
		if (!isValidRedirectCode(code))
		{
			std::stringstream ss;
			ss << "Invalid redirect code: " << code << " (must be between 300 and 399)";
			throw ValidatorException(ss.str());
		}

		if (location.getRedirectUrl().empty())
			throw ValidatorException("Redirect URL cannot be empty");
	}

	if (!location.getCgiExtensions().empty())
	{
		std::map<std::string, std::string>::const_iterator it;
		for (it = location.getCgiExtensions().begin(); it != location.getCgiExtensions().end(); ++it)
		{
			if (!isValidCgiExtension(it->first))
			{
				std::stringstream ss;
				ss << "CGI extension must start with dot: " << it->first;
				throw ValidatorException(ss.str());
			}
			if (!isValidCgiInterpreterPath(it->second))
			{
				std::stringstream ss;
				ss << "Invalid CGI interpreter path: " << it->second;
				throw ValidatorException(ss.str());
			}
		}
	}

	if (!location.getUploadStore().empty())
	{
		if (!location.hasMethod("POST"))
		{
			throw ValidatorException("upload_store requires POST method in location " + location.getPath());
		}
	}
}

void Validator::checkDuplicatePorts()
{
	std::set<int> ports;

	for (size_t i = 0; i < servers.size(); ++i)
	{
		int port = servers[i].getPort();
		
		if (ports.find(port) != ports.end())
		{
			std::stringstream ss;
			ss << "Duplicate port number: " << port;
			throw ValidatorException(ss.str());
		}
		
		ports.insert(port);
	}
}

bool Validator::isValidPort(int port) const
{
	return port > 0 && port <= 65535;
}

bool Validator::isValidRedirectCode(int code) const
{
	return code >= 300 && code <= 399;
}

bool Validator::isValidMethod(const std::string& method) const
{
	return method == "GET" || method == "POST" || method == "DELETE";
}

bool Validator::isValidCgiExtension(const std::string& ext) const
{
	return !ext.empty() && ext[0] == '.';
}

bool Validator::isValidCgiInterpreterPath(const std::string& path) const
{
	struct stat st;
	if (!path.empty() && stat(path.c_str(), &st) == 0 
		&& S_ISREG(st.st_mode) && access(path.c_str(), X_OK) == 0)
		return true;
	return false;
}
