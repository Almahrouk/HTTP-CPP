#include "Client.hpp"
#include "Logger.hpp"

Client::Client() : client_fd(-1), client_addr_len(sizeof(client_addr)) {}

