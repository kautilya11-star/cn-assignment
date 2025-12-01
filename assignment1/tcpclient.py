#!/usr/bin/env python3
import socket
import time

def run_tcp_client():
    # Create a TCP/IP socket
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    # Server address and port
    server_address = ('10.0.0.1', 12345)  # Server's IP address
    
    try:
        print(f'Connecting to server at {server_address[0]}:{server_address[1]}')
        
        # Connect to the server
        client_socket.connect(server_address)
        print('Connected to server successfully!')
        
        # Send "Hi" message to server
        message = 'Hi'
        print(f'Sending message: "{message}"')
        client_socket.sendall(message.encode('utf-8'))
        
        # Receive response from server
        response = client_socket.recv(1024).decode('utf-8')
        print(f'Received response from server: "{response}"')
        
    except Exception as e:
        print(f"Error: {e}")
    finally:
        # Clean up the connection
        client_socket.close()
        print('Connection closed')

if __name__ == '__main__':
    run_tcp_client()
