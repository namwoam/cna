// clib
#include <stdio.h>
#include <openssl/bio.h> /* BasicInput/Output streams */
#include <openssl/err.h> /* errors */
#include <openssl/ssl.h> /* core library */
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>

// cpp lib
#include <iostream>
#include <sstream>
#include <vector>

// openssl lib

#include <openssl/ssl.h>
#include <openssl/err.h>

#define buffer_size 1024
int client_fd;
int host_payment_fd;
pthread_t payment_thread;
pthread_mutex_t display_lock;

SSL_CTX *ctx, *server_ctx;
SSL *client_ssl;

void intHandler(int dummy)
{
    close(client_fd);
    close(host_payment_fd);
    printf("Client terminated\n");
    exit(EXIT_SUCCESS);
}

struct active_user
{
    std::string username, host;
    int port;
};

std::vector<struct active_user> active_users;

void *receive_payment(void *ptr)
{
    int hosting_port = *((int *)ptr);
    free(ptr);
    int opt = 1;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    printf("Receive payment hosting on port:%d\n", hosting_port);
    // Creating socket file descriptor
    if ((host_payment_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("create receive message socket failed\n");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(host_payment_fd, SOL_SOCKET,
                   SO_REUSEADDR, &opt,
                   sizeof(opt)))
    {
        perror("setsockopt failed\n");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(hosting_port);
    // Forcefully attaching socket to the port
    if (bind(host_payment_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("receive message socket bind failed\n");
        exit(EXIT_FAILURE);
    }
    if (listen(host_payment_fd, 3) < 0)
    {
        perror("receive message socket listen failed\n");
        exit(EXIT_FAILURE);
    }
    int connection_fd;
    char buffer[buffer_size];
    pthread_mutex_unlock(&display_lock);
    while (1)
    {
        if ((connection_fd = accept(host_payment_fd, (struct sockaddr *)&address,
                                    &addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        SSL *server_ssl = SSL_new(server_ctx);
        if (!server_ssl)
        {
            fprintf(stderr, "Failed to create SSL connection: %s\n", ERR_error_string(ERR_get_error(), NULL));
            exit(EXIT_FAILURE);
        }
        SSL_set_fd(server_ssl, connection_fd);
        printf("handshake started\n");
        if (SSL_accept(server_ssl) <= 0)
        {
            fprintf(stderr, "SSL handshake failed: %s\n", ERR_error_string(ERR_get_error(), NULL));
            exit(EXIT_FAILURE);
        }
        SSL_read(server_ssl, buffer, buffer_size - 1);
        std::cout << "Received payment:" << buffer << std::endl;
        std::string message = "received payment";
        SSL_write(server_ssl, message.c_str(), message.length());
        SSL_write(client_ssl, buffer, strlen(buffer));
        SSL_shutdown(server_ssl);
        SSL_free(server_ssl);
        close(connection_fd);
    }
}

int main(int argc, char *argv[])
{
    SSL_library_init();
    SSL_load_error_strings();
    ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx)
    {
        fprintf(stderr, "Failed to create SSL context: %s\n", ERR_error_string(ERR_get_error(), NULL));
        return 1;
    }
    server_ctx = SSL_CTX_new(TLS_server_method());
    if (!server_ctx)
    {
        fprintf(stderr, "Failed to create SSL context: %s\n", ERR_error_string(ERR_get_error(), NULL));
        exit(EXIT_FAILURE);
    }

    // Load the server's certificate and private key
    if (SSL_CTX_use_certificate_file(server_ctx, "servercert.pem", SSL_FILETYPE_PEM) <= 0)
    {
        fprintf(stderr, "Failed to load certificate: %s\n", ERR_error_string(ERR_get_error(), NULL));
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(server_ctx, "serverkey.pem", SSL_FILETYPE_PEM) <= 0)
    {
        fprintf(stderr, "Failed to load private key: %s\n", ERR_error_string(ERR_get_error(), NULL));
        exit(EXIT_FAILURE);
    }
    signal(SIGINT, intHandler);
    if (pthread_mutex_init(&display_lock, NULL) != 0)
    {
        perror("mutex init has failed\n");
        exit(EXIT_FAILURE);
    }
    while (1)
    {
        system("clear");
        printf("CNA socket client\n");
        std::string server_addr;
        std::cout << "Plese enter server <host>:<port> :";
        std::cin >> server_addr;
        std::string server_host;
        int server_port;
        try
        {
            server_host = server_addr.substr(0, server_addr.find(':'));
            server_port = std::stoi(server_addr.substr(server_addr.find(':') + 1));
        }
        catch (...)
        {
            perror("invalid format of server address\n");
            exit(EXIT_FAILURE);
        }
        // std::cout << "server host:" << server_host << " server port:" << server_port << std::endl;
        std::cout << "Establishing connection..." << std::endl;

        struct sockaddr_in serv_addr;
        if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("create socket failed\n");
            exit(EXIT_FAILURE);
        }
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(server_port);
        if (inet_pton(AF_INET, server_host.c_str(), &serv_addr.sin_addr) <= 0)
        {
            perror("invalid server address\n");
            exit(EXIT_FAILURE);
        }
        int status;
        if ((status = connect(client_fd, (struct sockaddr *)&serv_addr,
                              sizeof(serv_addr))) < 0)
        {
            perror("failed to establish connection\n");
            exit(status);
        }
        std::cout << "Successfully connected to socket server at " << server_host << ":" << server_port << std::endl;
        std::string username, payee_username;
        client_ssl = SSL_new(ctx);
        if (!client_ssl)
        {
            fprintf(stderr, "Failed to create SSL connection: %s\n", ERR_error_string(ERR_get_error(), NULL));
            exit(EXIT_FAILURE);
        }
        SSL_set_fd(client_ssl, client_fd);

        // Do the handshake
        if (SSL_connect(client_ssl) <= 0)
        {
            fprintf(stderr, "SSL handshake failed: %s\n", ERR_error_string(ERR_get_error(), NULL));
            exit(EXIT_FAILURE);
        }
        while (1)
        {
            std::cout << "========" << std::endl;
            std::cout << "Please select command:" << std::endl
                      << "1. Register" << std::endl
                      << "2. Login" << std::endl
                      << "3. Check server status" << std::endl
                      << "4. Terminate connection" << std::endl
                      << "5. Initiate payment" << std::endl;
            std::cout << "Command:";
            std::string command_string;
            std::cin >> command_string;
            int command;
            try
            {
                command = std::stoi(command_string);
            }
            catch (...)
            {
                command = -1;
            }
            std::cout << "========" << std::endl;
            std::string send_message;
            int payment_amount, port_num;
            if (command == 1)
            {
                std::cout << "Command: Register" << std::endl;
                std::cout << "Please enter your name:";
                std::cin >> username;
                send_message = std::string("REGISTER#") + username;
            }
            else if (command == 2)
            {
                std::cout << "Command: Login" << std::endl;
                std::cout << "Please enter your name:";
                std::cin >> username;
                std::cout << "Please enter port number for p2p communication:";
                std::cin >> port_num;
                send_message = username + std::string("#") + std::to_string(port_num);
            }
            else if (command == 3)
            {
                std::cout << "Command: Check server status" << std::endl;
                send_message = "List";
            }
            else if (command == 4)
            {
                std::cout << "Command: Terminate connection" << std::endl;
                send_message = "Exit";
                SSL_write(client_ssl, send_message.c_str(), send_message.length());
                close(client_fd);
                break;
            }
            else if (command == 5)
            {
                std::cout << "Command: Initiate payment" << std::endl;
                std::cout << "Available active users:";
                for (int i = 0; i < active_users.size(); i++)
                {
                    std::cout << active_users.at(i).username;
                    if (i == active_users.size() - 1)
                    {
                        std::cout << std::endl;
                    }
                    else
                    {
                        std::cout << ',';
                    }
                }
                std::cout << "Please enter payee username:";
                std::cin >> payee_username;
                std::cout << "Please enter payment amount:";
                std::cin >> payment_amount;
                struct active_user target;
                for (auto au : active_users)
                {
                    if (au.username == payee_username)
                    {
                        target = au;
                        break;
                    }
                }
                if (target.username == "")
                {
                    std::cout << "Payee unavailable" << std::endl;
                    continue;
                }
                struct sockaddr_in payee_addr;
                int payment_fd;
                if ((payment_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                {
                    printf("create payment socket failed\n");
                    continue;
                }
                payee_addr.sin_family = AF_INET;
                payee_addr.sin_port = htons(target.port);
                if (inet_pton(AF_INET, target.host.c_str(), &payee_addr.sin_addr) <= 0)
                {
                    printf("invalid payee server address\n");
                    continue;
                }
                int status;
                if ((status = connect(payment_fd, (struct sockaddr *)&payee_addr,
                                      sizeof(payee_addr))) < 0)
                {
                    printf("failed to establish payment connection\n");
                    continue;
                }
                SSL *ssl = SSL_new(ctx);
                if (!ssl)
                {
                    fprintf(stderr, "Failed to create SSL connection: %s\n", ERR_error_string(ERR_get_error(), NULL));
                    continue;
                }
                SSL_set_fd(ssl, payment_fd);
                if (SSL_connect(ssl) <= 0)
                {
                    fprintf(stderr, "SSL handshake failed: %s\n", ERR_error_string(ERR_get_error(), NULL));
                    continue;
                }
                std::string payment_message = username + std::string("#") + std::to_string(payment_amount) + std::string("#") + payee_username;
                SSL_write(ssl, payment_message.c_str(), payment_message.length());
                char p2p_buffer[buffer_size] = {0};
                SSL_read(ssl, p2p_buffer, buffer_size - 1);
                std::string p2p_receive_message = std::string(p2p_buffer);
                std::cout << p2p_receive_message << std::endl;
                SSL_shutdown(ssl);
                SSL_free(ssl);
                close(payment_fd);
                continue;
            }
            else
            {
                std::cout << "Invalid Command" << std::endl;
                continue;
            }
            // std::cout << "send_message:" << send_message << std::endl;
            SSL_write(client_ssl, send_message.c_str(), send_message.length());
            char buffer[buffer_size] = {0};
            SSL_read(client_ssl, buffer, buffer_size - 1);
            std::string receive_message = std::string(buffer);
            if (command == 1)
            {
                if (receive_message == "100 OK\n")
                {
                    std::cout << "Registeration success, now you can login to this server with username:" << username << std::endl;
                }
                else if (receive_message == "210 FAIL\n")
                {
                    std::cout << "Registeration failed, user already exist" << std::endl;
                }
                else
                {
                    std::cout << "Unrecognized server respond:" << receive_message << std::endl;
                }
            }
            else if (command == 2)
            {
                if (receive_message == "220 AUTH_FAIL\n")
                {
                    std::cout << "Login failed, user:" << username << " does not exist on the designated server" << std::endl;
                }
                else
                {
                    std::cout << "Login success, now you can check other online users" << std ::endl;
                    std::cout << "Setting up socket for receiving payment at port:" << port_num << std::endl;
                    int *arg = (int *)malloc(sizeof(*arg));
                    if (arg == NULL)
                    {
                        fprintf(stderr, "Couldn't allocate memory for thread arg.\n");
                        exit(EXIT_FAILURE);
                    }
                    *arg = port_num;
                    pthread_mutex_unlock(&display_lock);
                    pthread_mutex_lock(&display_lock);
                    int ret = pthread_create(&payment_thread, NULL, receive_payment, arg);
                    if (ret)
                    {
                        perror("failed to initialized thread\n");
                        exit(EXIT_FAILURE);
                    }
                    pthread_mutex_lock(&display_lock);
                }
            }
            else if (command == 3)
            {
                if (receive_message == "Please login first\n")
                {
                    std::cout << "Please login first" << std::endl;
                }
                else
                {
                    int balance, active_user;
                    char ch;
                    std::string public_key, user_information;
                    std::stringstream ss(receive_message);
                    ss >> balance;
                    ss.ignore();
                    std::getline(ss, public_key);
                    ss >> active_user;
                    std::cout << "Balance: " << balance << std::endl;
                    std::cout << "Server public key:" << public_key << std::endl;
                    std::cout << "Active user:" << active_user << std::endl;
                    std::cout << "Online users:" << std::endl;
                    active_users.clear();
                    for (int i = 0; i < active_user; i++)
                    {
                        ss >> user_information;
                        user_information[user_information.find('#')] = '@';
                        user_information[user_information.find('#')] = ':';
                        std::cout << user_information << std::endl;
                        struct active_user au;
                        au.username = user_information.substr(0, user_information.find('@'));
                        au.host = user_information.substr(user_information.find('@') + 1, user_information.find(":") - user_information.find("@") - 1);
                        au.port = std::stoi(user_information.substr(user_information.find(":") + 1));
                        active_users.push_back(au);
                    }
                }
            }
        }
    }
    SSL_CTX_free(ctx);
    SSL_CTX_free(server_ctx);
    ERR_free_strings();
    EVP_cleanup();

    return 0;
}