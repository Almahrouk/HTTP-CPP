#pragma once



# include <vector>
# include <map>
# include <algorithm>
# include <string>
# include <cstring>
# include <unistd.h>
# include <fstream>
# include <sstream>
# include <fcntl.h>
# include <iostream>
# include <cerrno>

# include <poll.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>

# include "../config/Parser.hpp"
# include "../request/HttpRequest.hpp"
# include "../request/RequestHandler.hpp"
# include "../client/Client.hpp"
# include "../cookie/Cookie.hpp"

# define TIMEOUT 10


class Server
{
private:
    std::vector<int>                    listenSockets;
    std::map<int, const ServerConfig>   configs;
    std::map<int, Client>               clients;
    std::map<int, CgiClient>            cgi_clients;
    std::vector<struct pollfd>          pollFds;

    bool    clientRemoved;

    Cookie  cookies;

private:
    int     addListeningSocket(int port, std::string host);
    void    bindSocket(int newServerSocket, int port, std::string host);
    bool    isListeningSocket(int fd);
    void    closeSocket(int fd);
    
    // HTTP
    void    acceptClient(int serverFd); // session creation
    void    readFromClient(int client_fd);
    void    writeToClient(int client_fd);
    void    changePollEvent(int fd, short int event);
    void    closeClient(int clientFd);
    void    sendErrorResponse(int client_fd, int serverFd, struct sockaddr_in client, int error_code);
    

    // CGI:
    void    make_cgi_pipes_nonblocking(int& script_in, int& script_out);
    void    add_cgi_pipes_to_pollFds(int script_in, int script_out);
    bool    isCgiPipe(int fd);
    void    processCgiEvent(int cgi_pipe);
    void    processCgiReadEvent(int   cgi_pipe);
    void    processCgiWriteEvent(int cgi_pipe);
    void    setHttpClientResponse(Cgi& cgi, int http_client_fd);
    void    close_cgi_client(int cgi_pipe_fd);

public:
    void    run();




public:
    Server(const std::vector<ServerConfig>& servers);
};

std::string intToString(int value);
std::string ipToString(uint32_t addr);