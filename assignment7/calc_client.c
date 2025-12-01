#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8888
#define MAX_BUFFER 1024
#define TIMEOUT 5

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[MAX_BUFFER];
    
    if(argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        printf("Example: %s 10.0.0.1\n", argv[0]);
        exit(1);
    }
    
    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    
    // Set timeout for receiving
    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    printf("UDP Calculator Client connected to %s:%d\n", argv[1], PORT);
    printf("Available operations:\n");
    printf("Single number: sin, cos, tan, log, sqrt, inv\n");
    printf("Two numbers: +, -, *, /, pow\n");
    printf("Format: operation number1 [number2]\n");
    printf("Type 'quit' to exit\n\n");
    
    int packet_num = 0;
    
    while(1) {
        printf("Enter operation: ");
        fgets(buffer, MAX_BUFFER, stdin);
        
        // Remove newline
        buffer[strcspn(buffer, "\n")] = 0;
        
        if(strcmp(buffer, "quit") == 0) {
            break;
        }
        
        packet_num++;
        printf("Sending packet #%d: %s\n", packet_num, buffer);
        
        // Send request to server
        if(sendto(sockfd, buffer, strlen(buffer), 0, 
                 (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("Send failed");
            continue;
        }
        
        // Receive response from server
        memset(buffer, 0, MAX_BUFFER);
        int recv_len = recvfrom(sockfd, buffer, MAX_BUFFER, 0, NULL, NULL);
        
        if(recv_len < 0) {
            printf("PACKET LOSS! No response from server (timeout after %d seconds)\n", TIMEOUT);
            printf("This demonstrates UDP unreliability\n\n");
        } else {
            printf("Server response: %s\n\n", buffer);
        }
        
        sleep(1); // Small delay between requests
    }
    
    close(sockfd);
    printf("Client closed\n");
    return 0;
}