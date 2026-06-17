#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <sstream>
#include <thread>

void handle_client(int client_fd) {
    char buf[1024];
    int num_bytes = recv(client_fd, buf, sizeof(buf), 0);
    if (num_bytes <= 0) {
        std::cerr << "Client disconnected";
        return;
    }
    std::string req(buf, num_bytes);
    auto s = req.substr(0, req.find("\r\n"));
    std::stringstream ss(s);
    std::string method, path;
    ss >> method >> path;    
    std::string res;
    if (path == "/") {
        res = "HTTP/1.1 200 OK\r\n\r\n";
    }
    else if (path.size() > 6 && path.substr(0, 6) == "/echo/") {
        auto str = path.substr(6);
        res = "HTTP/1.1 200 OK\r\n";
        res += "Content-Type: text/plain\r\n";
        res += "Content-Length: " + std::to_string(str.size()) + "\r\n\r\n";
        res += str;
    }
    else if (path == "/user-agent") {
        std::string user_agent = "";
        auto pos = req.find("User-Agent:");
        if (pos != std::string::npos) {
            size_t start = pos + strlen("User-Agent: ");
            size_t end = req.find("\r\n", start);
            user_agent = req.substr(start, end - start);
        }
        res = "HTTP/1.1 200 OK\r\n";
        res += "Content-Type: text/plain\r\n";
        res += "Content-Length: " + std::to_string(user_agent.size()) + "\r\n";
        res += "\r\n";
        res += user_agent;
    }
    else {
        res = "HTTP/1.1 404 Not Found\r\n\r\n";
    }
    send(client_fd, res.c_str(), res.size(), 0);
}

int main(int argc, char **argv) {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket\n";
        return 1;
    }
    
    // Since the tester restarts your program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt failed\n";
        return 1;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(4221);
    
    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
        std::cerr << "Failed to bind to port 4221\n";
        return 1;
    }
    
    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        std::cerr << "listen failed\n";
        return 1;
    }
    
    while (true) {
        std::cout << "Waiting for a client to connect...\n";
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);    
        int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
        std::cout << "Client connected\n";
        std::thread(handle_client, client_fd).detach();
    }

    close(server_fd);

    return 0;
}