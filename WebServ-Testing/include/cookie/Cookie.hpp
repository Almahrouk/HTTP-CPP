#pragma once


#include "../request/HttpRequest.hpp"
#include <map>
#include <cstdlib>

#define HALF_HOUR 1800

class Cookie
{
private:
    std::string token;
    std::string base;
    size_t expireTime;
    std::map<std::string , size_t > tokens;
    void    extractToken(const std::string& cookie);
    void    generateToken();

public:
    std::string key;
    std::string value;
    Cookie();
    bool    shouldSetCookie;
    void    checkRequest(const HttpRequest &req);

};