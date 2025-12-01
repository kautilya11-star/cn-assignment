import socket
import json

fruits = {"apple": 50, "banana": 30, "orange": 20}
clients = []

server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind(('localhost', 12346))

print("UDP Server running on port 12346...")

while True:
    data, addr = server_socket.recvfrom(1024)
    data = data.decode()
    
    client_id = str(addr[0]) + ":" + str(addr[1])
    if client_id not in clients:
        clients.append(client_id)
    
    if data == "list":
        response = ""
        for f, q in fruits.items():
            response += f + ": " + str(q) + "\n"
    
    elif data.startswith("buy"):
        parts = data.split()
        fruit = parts[1]
        quantity = int(parts[2])
        
        if fruit in fruits:
            if fruits[fruit] >= quantity:
                fruits[fruit] -= quantity
                response = "SUCCESS: " + str(quantity) + " " + fruit + " purchased. Unique customers: " + str(len(clients))
            else:
                response = "REGRET: Not enough " + fruit
        else:
            response = "ERROR: Fruit not available"
    
    elif data == "customers":
        response = "Total unique customers: " + str(len(clients)) + "\n" + "\n".join(clients)
    
    else:
        response = "Available commands: list, buy fruit quantity, customers"
    
    server_socket.sendto(response.encode(), addr)