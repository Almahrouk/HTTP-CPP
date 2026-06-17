# pragma once



# include "CgiClient.hpp"

#include <algorithm>


enum ParseState {
    READING_HEADERS,
    READING_BODY,
    COMPLETE,
    ERROR
};


class Client
{
public:
	time_t 				start_time;
    bool                timeout;
    // server config
    int                 http_fd; // UPDATE

    const ServerConfig& serverConfig;
    int                 clientPort;
    std::string         clientAddress;
    
    // request parsing
    std::string     request_str;
    HttpRequest     request;
    ParseState      state;
    size_t          header_end_pos;
    std::string     header_str;
    unsigned int    error_code;

    // cgi 
    bool                isCgi;
    bool                isCgiResponseError;
    Cgi*                cgi;

    // response
    std::string         response_str;
    HttpResponse        response;
    size_t              bytes_sent;
    
    Client(const ServerConfig& serverConfig, int clientPort, std::string clientAddress);
    ~Client();

    void    parseRequest();
    void    parseRequestHeaders();
    void    parseRequestBody();
};

std::string trim(const std::string& str);