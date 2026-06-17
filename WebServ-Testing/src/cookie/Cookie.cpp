#include "../../include/cookie/Cookie.hpp"
#include <iostream>
#include <sstream>
Cookie::Cookie(): token(""), base("0123456789ABCDEF"), shouldSetCookie(false) {}



void    Cookie::checkRequest(const HttpRequest &req)
{
    if (req.headers.find("cookie") != req.headers.end())
    {
        const std::map<std::string , std::string>::const_iterator it = req.headers.find("cookie");
        if (it != req.headers.end())
        {
            extractToken(it->second);
            if (tokens.find(token) != tokens.end())
            {
                if (time(NULL) - tokens.at(token) > HALF_HOUR)
                {
                    tokens.erase(token);
                    generateToken();
                }
                else
                    shouldSetCookie = false;
            }
            else
                generateToken();
        }
        else
            generateToken();
    }
    else
        generateToken();
}



void    Cookie::generateToken()
{
    expireTime = time(NULL);
    std::srand(expireTime);
    size_t t;
    token = "";
    
    for (int i = 0; i < 16; i++)
    {
        t = std::rand();
        std::stringstream ss;
        ss << base.at(t % 16);
        token.append(ss.str());
        t /= 16;
    }
    tokens.insert(std::make_pair(token, expireTime));
    shouldSetCookie = true;
    key = "Set-Cookie";
    value =  "sessionToken=" + token + "; Path=/; HttpOnly";
}



void    Cookie::extractToken(const std::string &cookie)
{
    size_t pos = cookie.find("sessionToken=");
    if (pos != std::string::npos)
        token = cookie.substr(pos + 13, 16);
    else
        token = "";
}


