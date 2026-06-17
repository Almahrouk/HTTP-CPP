#include "logger.hpp"
#include "server.hpp"

int main(void)
{
    log(INFO, "=== Web Server ===");
    Server server;
    
    if (server.init() != 0) {
        log(ERROR, "Server initialization failed");
        return EXIT_FAILURE;
    }
    if (server.listen() != 0) {
        log(ERROR, "Server listen failed");
        return EXIT_FAILURE;
    }
    server.run();
    return EXIT_SUCCESS;
}