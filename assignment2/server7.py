import socket
import threading

fruits = {"apple": 50, "banana": 30, "orange": 20}
clients = []

def handle_client(client_socket, addr):
    clients.append(str(addr[0]) + ":" + str(addr[1]))
    while True:
        data = client_socket.recv(1024).decode()
        if not data:
            break
        
        if data == "list":
            response = ""
            for f, q in fruits.items():
                response += f + ": " + str(q) + "\n"
        elif data.startswith("buy"):
            parts = data.split()
            fruit = parts[1]
            quantity = int(parts[2])
            if fruits[fruit] >= quantity:
                fruits[fruit] -= quantity
                response = "SUCCESS: " + str(quantity) + " " + fruit + " purchased. Unique customers: " + str(len(clients))
            else:
                response = "REGRET: Not enough " + fruit
        else:
            response = "Invalid command"
        
        client_socket.send(response.encode())
    client_socket.close()

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind(('localhost', 12345))
server.listen(5)
print("Server running...")

while True:
    client_socket, addr = server.accept()
    thread = threading.Thread(target=handle_client, args=(client_socket, addr))
    thread.start()