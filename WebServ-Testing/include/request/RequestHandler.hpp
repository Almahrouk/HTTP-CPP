#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include <string>
#include <vector>
#include "../config/ServerConfig.hpp"
#include "../config/LocationConfig.hpp"
#include "../response/HttpResponse.hpp"
#include "../logger/Logger.hpp"


class Client;

class RequestHandler
{
public:
    static HttpResponse handleRequest(const Client& client);

    static const LocationConfig*    findLocation(const std::string& uri,const ServerConfig& server); // i changed this method encapsulation to public
    static bool                     isCgiRequest(const std::string& uri,const LocationConfig* location); // this also 
    static void                     handleCgi(Client& client, const LocationConfig* location); // changed return type
    static HttpResponse             makeErrorResponse(int code,const ServerConfig& server);


private:
    static HttpResponse handleGet(const std::string& uri,const ServerConfig& server,const LocationConfig* location);
    static HttpResponse handlePost(const std::string& uri,const std::string& body,const ServerConfig& server,const LocationConfig* location);
    static HttpResponse handleDelete(const std::string& uri,const ServerConfig& server,const LocationConfig* location);

    static std::string buildFilePath(const std::string& uri,const ServerConfig& server,const LocationConfig* location);
    static std::string stripQueryString(const std::string& uri);


    static bool fileExists(const std::string& path);
    static bool directoryExists(const std::string& path);
    static std::string joinPath(const std::string& a, const std::string& b);
};

#endif
