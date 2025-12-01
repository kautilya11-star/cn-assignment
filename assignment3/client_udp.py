import socket

client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_address = ('localhost', 12346)

print("UDP Client connected to server...")

while True:
    cmd = raw_input("Enter command (list/buy fruit quantity/customers/quit): ")
    
    if cmd == "quit":
        break
    
    client_socket.sendto(cmd.encode(), server_address)
    response, server = client_socket.recvfrom(1024)
    print "Server:", response.decode()

client_socket.close()