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

void download_file(int sockfd, char *filename) {
    char buffer[BUFFER_SIZE];
    FILE *file;
    size_t total_bytes = 0;
    long file_size;
    clock_t start_time, end_time;
    char full_path[150];
    
    printf("Downloading file: %s\n", filename);
    
    // Send download command
    char command[100];
    snprintf(command, sizeof(command), "DOWNLOAD %s", filename);
    send(sockfd, command, strlen(command), 0);
    
    // Receive file size
    recv(sockfd, buffer, BUFFER_SIZE, 0);
    sscanf(buffer, "SIZE:%ld", &file_size);
    
    printf("File size: %ld bytes\n", file_size);
    
    // Create full path for saving
    snprintf(full_path, sizeof(full_path), "client_dir/%s", filename);
    
    // Open file for writing
    file = fopen(full_path, "wb");
    if (file == NULL) {
        perror("File create failed");
        return;
    }
    
    // Send ready signal
    send(sockfd, "READY", 5, 0);
    
    start_time = clock();
    
    // Receive file data
    while (total_bytes < file_size) {
        int bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }
        
        fwrite(buffer, 1, bytes_received, file);
        total_bytes += bytes_received;
    }
    
    end_time = clock();
    
    fclose(file);
    
    double transfer_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Download completed in %.4f seconds\n", transfer_time);
    printf("Total bytes received: %zu\n", total_bytes);
}

void upload_file(int sockfd, char *filename) {
    char buffer[BUFFER_SIZE];
    FILE *file;
    size_t bytes_read;
    clock_t start_time, end_time;
    char full_path[150];
    
    printf("Uploading file: %s\n", filename);
    
    // Create full path
    snprintf(full_path, sizeof(full_path), "client_dir/%s", filename);
    
    // Open file
    file = fopen(full_path, "rb");
    if (file == NULL) {
        perror("File open failed");
        return;
    }
    
    // Send upload command
    char command[100];
    snprintf(command, sizeof(command), "UPLOAD %s", filename);
    send(sockfd, command, strlen(command), 0);
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Receive server ready
    recv(sockfd, buffer, BUFFER_SIZE, 0);
    
    // Send file size
    char size_msg[50];
    snprintf(size_msg, sizeof(size_msg), "SIZE:%ld", file_size);
    send(sockfd, size_msg, strlen(size_msg), 0);
    
    // Wait for server ready signal
    recv(sockfd, buffer, BUFFER_SIZE, 0);
    
    start_time = clock();
    
    // Send file data
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (send(sockfd, buffer, bytes_read, 0) < 0) {
            perror("Send failed");
            break;
        }
    }
    
    end_time = clock();
    
    fclose(file);
    
    double transfer_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Upload completed in %.4f seconds\n", transfer_time);
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr;
    
    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        printf("Example: %s 10.0.0.1\n", argv[0]);
        exit(1);
    }
    
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    
    printf("Connecting to file server at %s:%d...\n", argv[1], PORT);
    
    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }
    
    printf("Connected to file server\n");
    printf("Client directory: client_dir/\n\n");
    
    // Download file from server
    printf("=== DOWNLOADING FILE FROM SERVER ===\n");
    download_file(sockfd, "server_file.txt");
    
    printf("\n=== UPLOADING FILE TO SERVER ===\n");
    upload_file(sockfd, "client_file.txt");
    
    close(sockfd);
    printf("\nFile transfer completed. Disconnected from server.\n");
    
    return 0;
}
