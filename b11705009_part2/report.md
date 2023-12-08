# Computer Networks and Applications, Fall 2023
## Programming Assignment Part 2
B11705009 An-Che Liang

## Compile

I use the `g++` compiler to complie the c++ source code into executable program. The command is as follow:

```bash
g++ -o server server.cpp
```

## Execution

The `server` program is a text-based terminal appilcation. To run the program, type `./server <port_number>` in the containing folder's terminal. Then a text-based interface will appear in the terminal if the provided port number is valid.

Once the server is successfully attachad to the socket with provided port number, it will spawn a new thread to listen to incoming clients. The program statically allocate memory for 64 concurrent threads, one thread will start when accepted a new connection and terminate when the client disconnects.

Also, the server program provides authentication capability. One client can only perform the `List` command once logged in, and will prevent ant other client to login to active accounts.

## Environment

This program is written and tested on the Ubuntu 22.04 linux distribution with x86-64 architecture CPU.

## Reference

1. GeekForGeeks [link](https://www.geeksforgeeks.org/socket-programming-cc/)