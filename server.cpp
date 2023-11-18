// c lib
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>

// cpp lib
#include <string>
#include <algorithm>
#include <iostream>
#include <vector>
#include <sstream>

#define worker_count 64
#define worker_idle 0
#define worker_working 1
#define buffer_size 1024
#define server_public_key "INFORMATION_MANAGEMENT"

pthread_t workers[worker_count];
int worker_status[worker_count] = {worker_idle};
int worker_sockets[worker_count];
char worker_buffer[worker_count][buffer_size] = {0};
int server_fd;

struct User
{
    std::string name;
    int balance = 10000;
    bool logged_in = false;
    std::string hostname;
    int p2p_port = -1;
};

std::vector<struct User> users;

void intHandler(int dummy)
{
    close(server_fd);
    printf("Server terminated\n");
    exit(EXIT_SUCCESS);
}

std::string generate_list(int user_index)
{
    if (user_index < 0 || user_index >= users.size() || users.at(user_index).logged_in == false)
    {
        return std::string("500 Internal Error\r\n");
    }
    std::stringstream ss;
    ss << users.at(user_index).balance << "\r\n";
    ss << server_public_key << "\r\n";
    std::vector<struct User> online_users;
    for (auto user : users)
    {
        if (user.logged_in == true)
        {
            online_users.push_back(user);
        }
    }
    ss << online_users.size() << "\r\n";
    for (auto user : online_users)
    {
        ss << user.name << '#' << user.hostname << '#' << user.p2p_port << "\r\n";
    }
    std::string result = ss.str();
    return result;
}

void *connection_work(void *ptr)
{
    int worker_index = *((int *)ptr);
    int login_status = -1;
    free(ptr);
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char client_host[32];
    int client_port;
    getsockname(worker_sockets[worker_index], (struct sockaddr *)&client_addr, &addr_len);
    inet_ntop(AF_INET, &client_addr.sin_addr, client_host, sizeof(client_host));
    client_port = ntohs(client_addr.sin_port);
    std::cout << "Receiving client form host:" << client_host << " port:" << client_port << std::endl;
    while (1)
    {
        int message_len = read(worker_sockets[worker_index], worker_buffer[worker_index], buffer_size - 1);
        if (message_len <= 0)
        {
            break;
        }
        std::cout << "message length:" << message_len << std::endl;
        std::string clean_message = std::string(worker_buffer[worker_index]);
        clean_message.resize(message_len);
        std::string return_message = "290 Server failed to handle\r\n";
        clean_message.erase(std::remove(clean_message.begin(), clean_message.end(), '\n'), clean_message.end());
        std::cout << "worker " << worker_index << " received: " << clean_message << std::endl;
        if (clean_message == "Exit")
        {

            return_message = "Bye\r\n";
            if (login_status >= 0)
            {
                users.at(login_status).logged_in = false;
            }
            send(worker_sockets[worker_index], return_message.c_str(), return_message.length(), 0);
            break;
        }
        else if (clean_message == "List")
        {
            std::cout << "login status:" << login_status << std::endl;
            if (login_status < 0)
            {
                return_message = "230 Unauthorized\r\n";
            }
            else
            {
                return_message = generate_list(login_status);
            }
        }
        else if (clean_message.find("REGISTER#") == 0)
        {
            std::string new_username = clean_message.substr(clean_message.find('#') + 1);
            bool error_flag = false;
            for (auto user : users)
            {
                if (user.name == new_username)
                {
                    error_flag = true;
                    break;
                }
            }
            if (error_flag)
            {
                return_message = "210 FAIL\r\n";
            }
            else
            {
                return_message = "100 OK\r\n";
                struct User new_user;
                new_user.name = new_username;
                users.push_back(new_user);
                printf("New user:%s\n", users.back().name.c_str());
            }
        }
        else if (std::count_if(clean_message.begin(), clean_message.end(), [](char c)
                               { return c == '#'; }) == 2)
        {
            std::string sender_name, amount_str, receiver_name;
            int parser_status = 0;
            for (auto ch : clean_message)
            {
                if (ch == '#')
                {
                    parser_status += 1;
                }
                else if (parser_status == 0)
                {
                    sender_name.push_back(ch);
                }
                else if (parser_status == 1)
                {
                    amount_str.push_back(ch);
                }
                else if (parser_status == 2)
                {
                    receiver_name.push_back(ch);
                }
            }
            if (login_status < 0)
            {
                return_message = "230 Unauthorized\r\n";
            }
            if (receiver_name != users.at(login_status).name)
            {
                return_message = "250 Forbidden\r\n";
            }
            else
            {
                int amount = std::stoi(amount_str);
                bool transaction_success = false;
                for (auto &user : users)
                {
                    if (user.name == sender_name)
                    {
                        user.balance -= amount;
                        users.at(login_status).balance += amount;
                        return_message = "100 OK\r\n";
                        transaction_success = true;
                        break;
                    }
                }
                if (transaction_success == false)
                {
                    return_message = "260 User Not Found\r\n";
                }
            }
        }
        else if (std::count_if(clean_message.begin(), clean_message.end(), [](char c)
                               { return c == '#'; }) == 1)
        {
            std::string username, port_string;
            int parser_status = 0;
            for (auto ch : clean_message)
            {
                if (ch == '#')
                {
                    parser_status = 1;
                }
                else if (parser_status == 0)
                {
                    username.push_back(ch);
                }
                else if (parser_status == 1)
                {
                    port_string.push_back(ch);
                }
            }
            int port = std::stoi(port_string);
            int user_index = 0;
            bool valid_user = false;
            for (auto &user : users)
            {
                if (user.name == username && user.logged_in == false && login_status < 0)
                {
                    user.logged_in = true;
                    user.p2p_port = port;
                    user.hostname = std::string(client_host);
                    login_status = user_index;
                    return_message = generate_list(login_status);
                    valid_user = true;
                    break;
                }
                user_index += 1;
            }
            if (valid_user == false)
            {
                return_message = "220 AUTH_FAIL\r\n";
            }
        }
        else
        {
            return_message = "240 Format Error\r\n";
        }
        send(worker_sockets[worker_index], return_message.c_str(), return_message.length(), 0);
    }
    printf("worker %d terminate\n", worker_index);
    worker_status[worker_index] = worker_idle;
    close(worker_sockets[worker_index]);
}

int main(int argc, char const *argv[])
{
    signal(SIGINT, intHandler);
    printf("CNA socket server\n");
    if (argc < 2)
    {
        perror("Port number not provided\n");
        exit(EXIT_FAILURE);
    }
    int hosting_port = atoi(argv[1]);
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
    address.sin_port = htons(hosting_port);
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
        for (int i = 0; i < worker_count; i++)
        {
            if (worker_status[i] == worker_idle)
            {
                printf("found idle worker: %d\n", i);
                if ((worker_sockets[i] = accept(server_fd, (struct sockaddr *)&address,
                                                &addrlen)) < 0)
                {
                    perror("accept new connection failed\n");
                    exit(EXIT_FAILURE);
                }
                printf("accepted new connection\n");
                int *arg = (int *)malloc(sizeof(*arg));
                if (arg == NULL)
                {
                    fprintf(stderr, "Couldn't allocate memory for thread arg.\n");
                    exit(EXIT_FAILURE);
                }
                *arg = i;
                int ret = pthread_create(&workers[i], NULL, connection_work, arg);
                if (ret)
                {
                    perror("failed to initialized a new worker\n");
                    exit(EXIT_FAILURE);
                }
                worker_status[i] = worker_working;
                break;
            }
        }
    }
    return 0;
}
