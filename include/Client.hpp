#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Common.hpp"

// enum EXIT_CLIENT
// {

// };

class Client
{
    private:
    public:
        int client_fd;
        struct sockaddr_in client_addr;
        socklen_t client_addr_len;
    std::string last_message;
    std::string response;
        Client();
};

// class Client
// {
//     private:
//         int client_fd;
//         struct sockaddr_in client_addr;
//         uint16_t client_port = 1234;
//         int connection_backlog;
//     public:
//         Client();
//         int init();
//         int listen();
//         int run();
// };

#endif