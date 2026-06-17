#include "common.h"

int main(int argc, char **argv)
{
    int                 server_socket, connfd, address_size;
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
        connfd = accept(server_socket, (SA *) &addr, &addr_len);
        inet_ntop(AF_INET, &addr, client_address, MAXLINE);
        printf("Connection from %s\n", client_address);
        memset(recvline, 0, MAXLINE);
        while((address_size = read(connfd, recvline, MAXLINE-1)) > 0)
        {
            char *hex = bin2hex(recvline, address_size);
            if (hex != NULL) {
                // fprintf(stdout, "\n%s\n\n%s", hex, recvline);
                fprintf(stdout, "\n%s", recvline);
                free(hex);
            } else {
                fprintf(stdout, "\n<failed to convert data>\n\n%s", recvline);
            }
            if (recvline[address_size-1] == '\n')
                break;
            memset(recvline, 0, MAXLINE);
        }
        if (address_size < 0)
            err_n_die("Read error!");
        snprintf((char *)buff, sizeof(buff), "HTTP/1.0 200 OK\r\n\r\nHello");
        write(connfd, (char *)buff, strlen((char *)buff));
        close(connfd);
    }
}