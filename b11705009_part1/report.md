# Computer Networks and Applications, Fall 2023
## Programming Assignment Part 1
B11705009 An-Che Liang

## Compile

I use the `gcc` compiler to complie the c++ source code into executable program. The command is as follow:

```bash
g++ -o client client.cpp
```

## Execution

The `client` program is a text-based terminal appilcation. To run the program, type `./client` in the containing folder's terminal. Then a text-based interface will appear in the terminal.

First, the client program will ask where to connect to the server application, include the IP address and port numeber. The client program will exit if it fails to connect to the socket.

Then, you will be given 5 commands to choose from:

1. Register
2. Login
3. Check server status (equivalent to List command on the server)
4. Terminate connection (equivalent to Exit command on the server)
5. Initiate payment

Enter the desired command accordingly, the client will ask additional information to perform some commands, such as it will ask for payee's name and payment amount when you initiate a new payment.

## Environment

This program is written and tested on the Ubuntu 22.04 linux distribution with x86-64 architecture CPU.

## Reference

1. GeekForGeeks [link](https://www.geeksforgeeks.org/socket-programming-cc/)
2. man7.org Linux manual page [link](https://man7.org/linux/man-pages/man3/pthread_create.3.html)
