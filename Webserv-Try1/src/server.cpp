#include "server.hpp"
#include "logger.hpp"

Server::Server() : server_fd(-1) {}

int Server::init()
{
    this->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->server_fd == -1) {
        log(ERROR, "Failed to create server socket");
        return CREATE_SOCKET_FAILURE;
    }
    int reuse = 1;
    if (setsockopt(this->server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        log(ERROR, "setsockopt failed");
        return SET_SOCKET_OPTION_FAILURE;
    }
    this->server_addr.sin_family = AF_INET;
    this->server_addr.sin_addr.s_addr = INADDR_ANY;
    this->server_addr.sin_port = htons(this->server_port);
    log(INFO, "Server initialized successfully");
    return 0;
}

int Server::listen()
{
    if (bind(this->server_fd, (struct sockaddr *) &this->server_addr, sizeof(this->server_addr)) != 0) {
        log(ERROR, "Failed to bind to port");
        return BING_PORT_FAILURE;
    }
    log(INFO, "Socket bound to port");
    this->connection_backlog = 5;
    if (::listen(this->server_fd, this->connection_backlog) != 0) {
        log(ERROR, "listen failed");
        return LISTEN_FAILURE;
    }
    log(INFO, "Server listening for connections");
    return 0;
}

int Server::run()
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    log(INFO, "Waiting for a client to connect...");
    int client_fd = accept(this->server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
    if (client_fd == -1) {
        log(ERROR, "accept failed");
        close(this->server_fd);
        return 1;
    }
    log(INFO, "Client connected");
    close(this->server_fd);
    return 0;
}

/*
int socket(int domain, int type, int protocol)
    domain: AF_INET (IPv4)
    type: SOCK_STREAM (TCP)
    protocol: 0 (default protocol for the given domain and type)
socket() creates an endpoint for communication and returns a file descriptor that  refers to that endpoint. 





*/