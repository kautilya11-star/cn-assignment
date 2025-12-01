#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8888
#define BUFFER_SIZE 1024

int sockfd;

// Function to receive messages from server
void *receive_handler(void *arg) {
    char buffer[BUFFER_SIZE];
    
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int recv_len = recv(sockfd, buffer, BUFFER_SIZE, 0);
        
        if (recv_len <= 0) {
            printf("\nDisconnected from server.\n");
            exit(1);
        }
        
        printf("\r%s\n", buffer);
        printf("You: ");
        fflush(stdout);
    }
    
    return NULL;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char username[50];
    pthread_t recv_thread;
    
    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        printf("Example: %s 127.0.0.1\n", argv[0]);
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
    
    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }
    
    printf("Connected to chat server at %s:%d\n", argv[1], PORT);
    
    // Get username
    printf("Enter your username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0; // Remove newline
    
    // Send username to server
    send(sockfd, username, strlen(username), 0);
    
    // Create thread for receiving messages
    if (pthread_create(&recv_thread, NULL, receive_handler, NULL) != 0) {
        perror("Thread creation failed");
        close(sockfd);
        exit(1);
    }
    
    printf("Type your messages (type '/quit' to exit):\n");
    
    // Send messages loop
    while (1) {
        printf("You: ");
        fflush(stdout);
        
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        
        // Remove newline
        buffer[strcspn(buffer, "\n")] = 0;
        
        // Check for quit command
        if (strcmp(buffer, "/quit") == 0) {
            send(sockfd, buffer, strlen(buffer), 0);
            break;
        }
        
        // Send message to server
        if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
            perror("Send failed");
            break;
        }
    }
    
    close(sockfd);
    printf("Disconnected from chat server.\n");
    return 0;
}
