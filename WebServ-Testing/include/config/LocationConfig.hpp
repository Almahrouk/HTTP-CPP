#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <string>
#include <vector>
#include <map>
class LocationConfig
{
private:
	std::string path;
	std::vector<std::string> methods;
	std::string root;
	std::string index;
	bool autoindex;
	std::string uploadStore;
	std::map<std::string, std::string> cgiExtensions;
	int redirectCode;
	std::string redirectUrl;

public:
	LocationConfig();
	LocationConfig(const std::string& path);
	LocationConfig(const LocationConfig& other);
	LocationConfig& operator=(const LocationConfig& other);
	~LocationConfig();

	const std::string& getPath() const;
	const std::vector<std::string>& getMethods() const;
	const std::string& getRoot() const;
	const std::string& getIndex() const;
	bool getAutoindex() const;
	const std::string& getUploadStore() const;
	const std::map<std::string, std::string>& getCgiExtensions() const;
	int getRedirectCode() const;
	const std::string& getRedirectUrl() const;

	void setPath(const std::string& path);
	void addMethod(const std::string& method);
	void setRoot(const std::string& root);
	void setIndex(const std::string& index);
	void setAutoindex(bool autoindex);
	void setUploadStore(const std::string& uploadStore);
	void addCgiExtension(const std::string& ext, const std::string& interpreter);
	void setRedirect(int code, const std::string& url);

	bool hasMethod(const std::string& method) const;
	bool hasRedirect() const;
};

#endif
