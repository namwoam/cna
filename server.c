#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
    printf("CNA socket server\n");
    if (argc < 2)
    {
        perror("Port number not provided\n");
        exit(EXIT_FAILURE);
    }
    int hosting_port = atoi(argv[1]);
    int server_fd;
    int opt = 1;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    printf("Hosting on port:%d\n", hosting_port);
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("create socket failed\n");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR, &opt,
                   sizeof(opt)))
    {
        perror("setsockopt failed\n");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = hosting_port;
    // Forcefully attaching socket to the port
    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("bind failed\n");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen failed\n");
        exit(EXIT_FAILURE);
    }
    printf("Listening for new client\n");
    while (1)
    {
        int connection_fd;
        if ((connection_fd = accept(server_fd, (struct sockaddr *)&address,
                                    &addrlen)) < 0)
        {
            perror("accept new connection failed\n");
            exit(EXIT_FAILURE);
        }
        else
        {
        }
    }
    close(server_fd);
    printf("Server terminated\n");
    return 0;
}
