#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 8888
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int client_sockets[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to get current timestamp
void get_timestamp(char *timestamp) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", t);
}

// Function to log message to file
void log_message(char *username, char *message) {
    FILE *file = fopen("chat_log.txt", "a");
    if (file == NULL) {
        perror("Failed to open log file");
        return;
    }
    
    char timestamp[20];
    get_timestamp(timestamp);
    fprintf(file, "[%s] %s: %s\n", timestamp, username, message);
    fclose(file);
}

// Function to broadcast message to all clients
void broadcast_message(char *username, char *message, int sender_socket) {
    pthread_mutex_lock(&mutex);
    
    char broadcast_msg[BUFFER_SIZE];
    snprintf(broadcast_msg, BUFFER_SIZE, "[%s]: %s", username, message);
    
    // Log the message
    log_message(username, message);
    
    // Send to all connected clients except sender
    for (int i = 0; i < client_count; i++) {
        if (client_sockets[i] != sender_socket) {
            if (send(client_sockets[i], broadcast_msg, strlen(broadcast_msg), 0) < 0) {
                perror("Send failed");
            }
        }
    }
    
    pthread_mutex_unlock(&mutex);
}

// Function to handle each client
void *client_handler(void *socket_desc) {
    int client_socket = *(int*)socket_desc;
    char username[50];
    char buffer[BUFFER_SIZE];
    
    // Get username from client
    if (recv(client_socket, username, sizeof(username), 0) <= 0) {
        close(client_socket);
        pthread_exit(NULL);
    }
    
    printf("Client connected: %s\n", username);
    
    // Welcome message
    char welcome_msg[BUFFER_SIZE];
    snprintf(welcome_msg, BUFFER_SIZE, "Welcome to chat room, %s! Type your messages below:", username);
    send(client_socket, welcome_msg, strlen(welcome_msg), 0);
    
    // Notify others about new user
    char join_msg[BUFFER_SIZE];
    snprintf(join_msg, BUFFER_SIZE, "%s has joined the chat!", username);
    broadcast_message("Server", join_msg, client_socket);
    
    // Chat loop
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int read_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
        
        if (read_size <= 0) {
            // Client disconnected
            printf("Client disconnected: %s\n", username);
            
            // Remove client from array
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < client_count; i++) {
                if (client_sockets[i] == client_socket) {
                    for (int j = i; j < client_count - 1; j++) {
                        client_sockets[j] = client_sockets[j + 1];
                    }
                    client_count--;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);
            
            // Notify others about user leaving
            char leave_msg[BUFFER_SIZE];
            snprintf(leave_msg, BUFFER_SIZE, "%s has left the chat!", username);
            broadcast_message("Server", leave_msg, client_socket);
            
            break;
        }
        
        // Remove newline character
        buffer[strcspn(buffer, "\n")] = 0;
        
        // Check if user wants to quit
        if (strcmp(buffer, "/quit") == 0) {
            printf("Client quit: %s\n", username);
            
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < client_count; i++) {
                if (client_sockets[i] == client_socket) {
                    for (int j = i; j < client_count - 1; j++) {
                        client_sockets[j] = client_sockets[j + 1];
                    }
                    client_count--;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);
            
            char leave_msg[BUFFER_SIZE];
            snprintf(leave_msg, BUFFER_SIZE, "%s has left the chat!", username);
            broadcast_message("Server", leave_msg, client_socket);
            
            break;
        }
        
        // Broadcast the message to all clients
        printf("%s: %s\n", username, buffer);
        broadcast_message(username, buffer, client_socket);
    }
    
    close(client_socket);
    pthread_exit(NULL);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t thread_id;
    
    // Initialize client sockets array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }
    
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
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
    
    printf("Chat Server started on port %d...\n", PORT);
    printf("Waiting for clients to connect...\n\n");
    
    // Create chat log file
    FILE *file = fopen("chat_log.txt", "w");
    if (file != NULL) {
        char timestamp[20];
        get_timestamp(timestamp);
        fprintf(file, "=== Chat Session Started at %s ===\n", timestamp);
        fclose(file);
    }
    
    // Main server loop
    while (1) {
        // Accept new connection
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        
        printf("New connection from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // Add client to array
        pthread_mutex_lock(&mutex);
        if (client_count < MAX_CLIENTS) {
            client_sockets[client_count] = client_socket;
            client_count++;
        } else {
            printf("Max clients reached. Rejecting connection.\n");
            close(client_socket);
            pthread_mutex_unlock(&mutex);
            continue;
        }
        pthread_mutex_unlock(&mutex);
        
        // Create thread for client
        if (pthread_create(&thread_id, NULL, client_handler, (void*)&client_socket) < 0) {
            perror("Thread creation failed");
            close(client_socket);
            
            // Remove client from array
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < client_count; i++) {
                if (client_sockets[i] == client_socket) {
                    for (int j = i; j < client_count - 1; j++) {
                        client_sockets[j] = client_sockets[j + 1];
                    }
                    client_count--;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);
        }
        
        // Detach thread so resources are freed when done
        pthread_detach(thread_id);
    }
    
    close(server_socket);
    return 0;
}
