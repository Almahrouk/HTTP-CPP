#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

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
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  std::cout << "Waiting for a client to connect...\n";
  
  int client = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  std::cout << "Client connected\n";

  const size_t message_size = 1024;
  std::string message(message_size, '\0');

  if (recv(client, (void *)&message[0], message.max_size(), 0) == -1){
    std::cerr << "Listen failed\n";
    return 1;
  }

  std::string path;
  size_t methodEnd = message.find(' ');
  if (methodEnd != std::string::npos) {
    auto start = methodEnd + 1;
    auto end = message.find(' ', start);
    if (end != std::string::npos) {
        path = message.substr(start, end - start);
    }
  }

  std::string response;
  if (path == "/") {
    response = "HTTP/1.1 200 OK\r\n\r\n";
  } else if(path.find("/echo/") == 0) {
    std::string content = path.substr(6);
    response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(content.size()) + "\r\n\r\n" + content;
  } else {
    response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
  }

  send(client, response.c_str(), response.length(), 0);
  
  close(server_fd);

  return 0;
}
