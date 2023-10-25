import socket
import argparse
import threading
import re
parser = argparse.ArgumentParser()
parser.add_argument("port", help="the server listen on this port",
                    type=int)
g = parser.add_mutually_exclusive_group()
g.add_argument("-d",help="simple output" , action="store_true")
g.add_argument("-s",help="show online list" , action="store_true")
g.add_argument("-a",help="show messages" , action="store_true")
args = parser.parse_args()

workers = []
users = []
balance = []
online_user_ports = {}

def thread_job(socket):
    while True:
        request = socket.recv(1024)
        request:str = request.decode("utf-8").replace("\n" , "") # convert bytes to string
        # if we receive "close" from the client, then we break
        # out of the loop and close the conneciton
        print(f"Received: {request}")
        if request == "Exit":
            # send response to the client which acknowledges that the
            # connection should be closed and break out of the loop
            socket.send("Bye\n".encode("utf-8"))
            break
        elif request == "List":
            pass
        elif request.startswith("REGISTER"):
            _ , account_name = request.split("#")
            if account_name in users:
                response = "210 FAIL"
            else:
                response = "100 OK"
                users.append(account_name)
                balance.append(10000)
        elif request.startswith():
            pass
        # convert and send accept response to the client
        socket.send((response+"\n").encode("utf-8"))
    print(f"Connection for client:{socket.getpeername()} closed")
    socket.close()
    
def run_server()->None:
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_ip = "127.0.0.1"
    server_port = args.port
    # bind the socket to a specific address and port
    server.bind((server_ip, server_port))
    server.listen(0)
    print(f"Listening on {server_ip}:{server_port}")
    try:
        while True:
            client_socket, _ = server.accept()
            print(f"Accepted connection for client:{client_socket.getpeername()}")
            workers.append(threading.Thread(target=thread_job , args=(client_socket,)))
            workers[-1].start()
    except KeyboardInterrupt:
        server.close()

run_server()