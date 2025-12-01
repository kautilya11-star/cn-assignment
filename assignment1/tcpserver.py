#!/usr/bin/env python3
import socket

def run_tcp_server():
    # Create a TCP/IP socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    # Allow reuse of address
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    # Bind the socket to the address and port
    server_address = ('10.0.0.1', 12345)  # Using host1's IP
    print(f'Starting TCP server on {server_address[0]}:{server_address[1]}')
    server_socket.bind(server_address)
    
    # Listen for incoming connections
    server_socket.listen(1)
    print('Server is listening for connections...')
    
    try:
        while True:
            # Wait for a connection
            print('Waiting for a connection...')
            client_socket, client_address = server_socket.accept()
            print(f'Connection established from {client_address}')
            
            try:
                # Receive data from client
                data = client_socket.recv(1024).decode('utf-8')
                print(f'Received from client: "{data}"')
                
                if data == 'Hi':
                    # Send response to client
                    response = 'Hello'
                    client_socket.sendall(response.encode('utf-8'))
                    print(f'Sent response: "{response}"')
                else:
                    response = 'Expected "Hi"'
                    client_socket.sendall(response.encode('utf-8'))
                    
            except Exception as e:
                print(f"Error handling client: {e}")
            finally:
                # Clean up the connection
                client_socket.close()
                print('Connection closed\n')
                
    except KeyboardInterrupt:
        print("\nServer shutting down...")
    finally:
        server_socket.close()

if __name__ == '__main__':
    run_tcp_server()
