#include "logger.hpp"
#include "server.hpp"
// #include "ConfigParsing.hpp"

int main(void)
{
    // log(INFO, "=== Tokenization ===");
    // if (ac != 2)
    // {
    //     log(ERROR, "Usage ./webserve <Configuration File>");
    //     return (EXIT_FAILURE);
    // }
    // ConfigParsing config(av[1]);
    // if (config.Check_File() != EXIT_SUCCESS)
    // {
    //     return (EXIT_FAILURE);
    // }

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