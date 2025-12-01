#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8888
#define BUFFER_SIZE 1024

void send_file(int client_socket, char *filename) {
    char buffer[BUFFER_SIZE];
    FILE *file;
    size_t bytes_read;
    clock_t start_time, end_time;
    
    printf("Sending file: %s\n", filename);
    
    // Open file
    file = fopen(filename, "rb");
    if (file == NULL) {
        perror("File open failed");
        send(client_socket, "ERROR: File not found", 22, 0);
        return;
    }
    
    // Send file size first
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char size_msg[50];
    snprintf(size_msg, sizeof(size_msg), "SIZE:%ld", file_size);
    send(client_socket, size_msg, strlen(size_msg), 0);
    
    // Wait for client ready signal
    recv(client_socket, buffer, BUFFER_SIZE, 0);
    
    start_time = clock();
    
    // Send file data
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (send(client_socket, buffer, bytes_read, 0) < 0) {
            perror("Send failed");
            break;
        }
    }
    
    end_time = clock();
    
    fclose(file);
    
    double transfer_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("File sent successfully in %.4f seconds\n", transfer_time);
}

void receive_file(int client_socket, char *filename) {
    char buffer[BUFFER_SIZE];
    FILE *file;
    size_t total_bytes = 0;
    long file_size;
    clock_t start_time, end_time;
    
    printf("Receiving file: %s\n", filename);
    
    // Open file for writing
    file = fopen(filename, "wb");
    if (file == NULL) {
        perror("File create failed");
        return;
    }
    
    // Receive file size first
    recv(client_socket, buffer, BUFFER_SIZE, 0);
    sscanf(buffer, "SIZE:%ld", &file_size);
    
    printf("File size: %ld bytes\n", file_size);
    
    // Send ready signal
    send(client_socket, "READY", 5, 0);
    
    start_time = clock();
    
    // Receive file data
    while (total_bytes < file_size) {
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }
        
        fwrite(buffer, 1, bytes_received, file);
        total_bytes += bytes_received;
    }
    
    end_time = clock();
    
    fclose(file);
    
    double transfer_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("File received successfully in %.4f seconds\n", transfer_time);
    printf("Total bytes transferred: %zu\n", total_bytes);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    
    // Listen for connections
    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        exit(1);
    }
    
    printf("File Server started on port %d...\n", PORT);
    printf("Server directory: server_dir/\n");
    
    while (1) {
        // Accept client connection
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        
        printf("\nClient connected from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // Receive command from client
        int cmd_len = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (cmd_len <= 0) {
            close(client_socket);
            continue;
        }
        
        buffer[cmd_len] = '\0';
        
        if (strncmp(buffer, "DOWNLOAD", 8) == 0) {
            // Client wants to download a file
            char filename[100];
            sscanf(buffer, "DOWNLOAD %s", filename);
            char full_path[150];
            snprintf(full_path, sizeof(full_path), "server_dir/%s", filename);
            send_file(client_socket, full_path);
        }
        else if (strncmp(buffer, "UPLOAD", 6) == 0) {
            // Client wants to upload a file
            char filename[100];
            sscanf(buffer, "UPLOAD %s", filename);
            char full_path[150];
            snprintf(full_path, sizeof(full_path), "server_dir/%s", filename);
            receive_file(client_socket, full_path);
        }
        
        close(client_socket);
        printf("Client disconnected\n");
    }
    
    close(server_socket);
    return 0;
}