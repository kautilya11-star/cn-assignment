import socket

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect(('localhost', 12345))

while True:
    cmd = raw_input("Enter command (list/buy fruit quantity/quit): ")
    if cmd == "quit":
        break
    client.send(cmd.encode())
    response = client.recv(1024).decode()
    print "Server:", response

client.close()