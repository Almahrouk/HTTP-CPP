#include "common.h"

void err_n_die(const char *fmt, ...)
{
    int errno_save;
    va_list ap;

    // Save errno in case it's changed by this function
    errno_save = errno;

    // Print the formatted error message to stderr
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    fflush(stderr);
    
    if (errno_save != 0) {
        fprintf(stderr, "(errno = %d) : %s\n", errno_save, strerror(errno_save));
        strerror(errno_save);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    va_end(ap);

    exit(EXIT_FAILURE);
}

char *bin2hex(const unsigned char *input, size_t len)
{
    char *result;
    char *hexits = "0123456789ABCDEF";
    int result_len = len * 3 + 1;

    if (input == NULL || len == 0) {
        return NULL;
    }
    result = (char *)malloc(result_len);
    if (result == NULL) {
        return NULL;
    }
    bzero(result, result_len);
    for (size_t i = 0; i < len; i++) {
        result[i * 3] = hexits[input[i] >> 4];
        result[(i * 3) + 1] = hexits[input[i] & 0x0F];
        result[(i * 3) + 2] = ' ';
    }
    return result;
}

void handle_connection(int client_socket)
{
    char buffer[BUFSIZE];
    size_t bytes_read;
    int message_count = 0;
    char actual_path[PATH_MAX+1];

    // Read the filename request from client
    while((bytes_read = read(client_socket, buffer + message_count, sizeof(buffer) - message_count - 1)) > 0) {
        message_count += bytes_read;
        if (message_count >= BUFSIZE-1 || buffer[message_count-1] == '\n')
            break;
        if (bytes_read < 0) {
            fprintf(stderr, "Error reading from client socket: %s\n", strerror(errno));
            close(client_socket);
            return;
        }
    }
    
    // Null-terminate and trim newline
    buffer[message_count-1] = 0;
    printf("Received message: %s\n", buffer);
    fflush(stdout);

    // Resolve the file path
    if (realpath(buffer, actual_path) == NULL) {
        fprintf(stderr, "Error resolving path: %s\n", strerror(errno));
        close(client_socket);
        return;
    }
    
    // Open and send file contents
    FILE *file = fopen(actual_path, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file: %s\n", strerror(errno));
        close(client_socket);
        return;
    }
    
    while ((bytes_read = fread(buffer, 1, BUFSIZE, file)) > 0) {
        printf("Read %zu bytes from file\n", bytes_read);
        write(client_socket, buffer, bytes_read);
    }
    
    fclose(file);
    close(client_socket);
    printf("Finished handling connection\n");
}