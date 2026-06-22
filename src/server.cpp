#include "Server.hpp"
#include "Logger.hpp"
#include "Client.hpp"

Server::Server() : server_fd(-1), server_port(1234) {}

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
    // sleep(1000000);
    // exit(EXIT_FAILURE);
    this->connection_backlog = 5;
    if (::listen(this->server_fd, this->connection_backlog) != 0) {
        log(ERROR, "listen failed");
        return LISTEN_FAILURE;
    }
    log(INFO, "Server listening for connections");
    // sleep(1000000); // The OS is handling the TCP handshake (Kernal).
    return 0;
}

Client *Server::accept()
{
    Client *client = new Client();
    log(INFO, "Waiting for a client to connect...");

    client->client_addr_len = sizeof(client->client_addr);
    client->client_fd = ::accept(this->server_fd, (struct sockaddr *)&client->client_addr, &client->client_addr_len);
    if (client->client_fd == -1) {
        log(ERROR, "accept failed");
        close(this->server_fd);
        delete client;
        return NULL;
    }
    log(INFO, "Client connected");
    return client;
}

int Server::send_response(Client *client)
{
    std::string response = "HTTP/1.1 200 OK\r\n\r\n";
    // write(client_fd, response.data(), response.size());
    send(client->client_fd, response.c_str(), response.size(), 0);
    // sleep(1000000);
    return EXIT_SUCCESS;
}

int Server::recv(Client *client)
{
    std::string client_message(1024, '\0');
    ssize_t brecvd = ::recv(client->client_fd, (void *)&client_message[0], client_message.size(), 0);
    if (brecvd < 0)
    {
        std::cerr << "error receiving message from client\n";
        close(client->client_fd);
        close(this->server_fd);
        return 1;
    }
    client_message.resize(brecvd);
    std::cerr << "Client Message (length: " << client_message.size() << ")" << std::endl;
    std::clog << client_message << std::endl;
    client->last_message = client_message;
    client->response = client_message.find("GET / HTTP/1.1\r\n") == 0 ? "HTTP/1.1 200 OK\r\n\r\n" : "HTTP/1.1 404 Not Found\r\n\r\n";
    return 0;
}

int Server::send2(Client *client)
{
    ssize_t bsent = ::send(client->client_fd, client->response.c_str(), client->response.size(), 0);
    if (bsent < 0)
    {
        std::cerr << "error sending response to client\n";
        close(client->client_fd);
        close(this->server_fd);
        return 1;
    }
    return 0;
}

int Server::run()
{
    Client *client;

    while (true)
    {
        client = accept();
        if (!client) {
            log(ERROR, "run: accept failed");
            continue;
        }
        if (send_response(client) != 0)
            log(ERROR, "run: send_response failed");
        if (recv(client) != 0)
            log(ERROR, "run: recv failed");
        if (send2(client) != 0)
            log(ERROR, "run: send2 failed");
        close(client->client_fd);
        delete client;
    }
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