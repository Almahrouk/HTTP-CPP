#include "common.h"

int main(int argc, char **argv)
{
    int                 server_socket, client_socket, address_size;
    struct sockaddr_in  server_addr;
    uint8_t             buff[MAXLINE+1];
    uint8_t             recvline[MAXLINE+1];

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) // allocate socket resources [internet, Stream File, TCP]
        err_n_die("socket error!");
    // Address the connect to my machine
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family         = AF_INET;
    server_addr.sin_addr.s_addr    = htonl(INADDR_ANY);
    server_addr.sin_port           = htons(SERVER_PORT);
    // This socket is going to listen to this address
    if ((bind(server_socket, (SA *) &server_addr, sizeof(server_addr))) < 0)
        err_n_die("Bind error!");
    if ((listen(server_socket, 10)) < 0)
        err_n_die("Listen error!");
    while(1)
    {
        struct sockaddr_in  addr;
        socklen_t addr_len;
        char client_address[MAXLINE+1];

        printf("Waiting for a connection on port %d\n", SERVER_PORT);
        fflush(stdout);
        // int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
        client_socket = accept(server_socket, (SA *) &addr, &addr_len);
        handle_connection(client_socket);
    }
    return 0;
}