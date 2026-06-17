#include "config/LocationConfig.hpp"

LocationConfig::LocationConfig()
	: path(""),
	  methods(),
	  root(""),
	  index(""),
	  autoindex(false),
	  uploadStore(""),
	  cgiExtensions(),
	  redirectCode(0),
	  redirectUrl("")
{
}

LocationConfig::LocationConfig(const std::string& path)
	: path(path),
	  methods(),
	  root(""),
	  index(""),
	  autoindex(false),
	  uploadStore(""),
	  cgiExtensions(),
	  redirectCode(0),
	  redirectUrl("")
{
}

LocationConfig::LocationConfig(const LocationConfig& other)
	: path(other.path),
	  methods(other.methods),
	  root(other.root),
	  index(other.index),
	  autoindex(other.autoindex),
	  uploadStore(other.uploadStore),
	  cgiExtensions(other.cgiExtensions),
	  redirectCode(other.redirectCode),
	  redirectUrl(other.redirectUrl)
{
}

LocationConfig& LocationConfig::operator=(const LocationConfig& other)
{
	if (this != &other)
	{
		path = other.path;
		methods = other.methods;
		root = other.root;
		index = other.index;
		autoindex = other.autoindex;
		uploadStore = other.uploadStore;
		cgiExtensions = other.cgiExtensions;
		redirectCode = other.redirectCode;
		redirectUrl = other.redirectUrl;
	}
	return *this;
}

LocationConfig::~LocationConfig()
{
}

/* Getters */
const std::string& LocationConfig::getPath() const { return path; }
const std::vector<std::string>& LocationConfig::getMethods() const { return methods; }
const std::string& LocationConfig::getRoot() const { return root; }
const std::string& LocationConfig::getIndex() const { return index; }
bool LocationConfig::getAutoindex() const { return autoindex; }
const std::string& LocationConfig::getUploadStore() const { return uploadStore; }
const std::map<std::string, std::string>& LocationConfig::getCgiExtensions() const { return cgiExtensions; }
int LocationConfig::getRedirectCode() const { return redirectCode; }
const std::string& LocationConfig::getRedirectUrl() const { return redirectUrl; }

void LocationConfig::setPath(const std::string& path) { this->path = path; }
void LocationConfig::addMethod(const std::string& method) { methods.push_back(method); }
void LocationConfig::setRoot(const std::string& root) { this->root = root; }
void LocationConfig::setIndex(const std::string& index) { this->index = index; }
void LocationConfig::setAutoindex(bool autoindex) { this->autoindex = autoindex; }
void LocationConfig::setUploadStore(const std::string& uploadStore) { this->uploadStore = uploadStore; }
void LocationConfig::addCgiExtension(const std::string& ext, const std::string& interpreter)
{
	cgiExtensions[ext] = interpreter;
}

void LocationConfig::setRedirect(int code, const std::string& url)
{
	redirectCode = code;
	redirectUrl = url;
}

bool LocationConfig::hasMethod(const std::string& method) const
{
	for (size_t i = 0; i < methods.size(); ++i)
	{
		if (methods[i] == method)
			return true;
	}
	return false;
}

bool LocationConfig::hasRedirect() const
{
	return redirectCode != 0;
}