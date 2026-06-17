#ifndef SERVER_HPP
#define SERVER_HPP

#include "common.hpp"

enum EXIT_SERVER
{
    CREATE_SOCKET_FAILURE, 
    SET_SOCKET_OPTION_FAILURE, 
    BING_PORT_FAILURE,
    LISTEN_FAILURE
};

class Server
{
    private:
        int server_fd;
        struct sockaddr_in server_addr;
        uint16_t server_port = 1234;
        int connection_backlog;
    public:
        Server();
        int init();
        int listen();
        int run();
};

#endif