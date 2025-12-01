#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>  // ADD THIS LINE
#include <math.h>

#define PORT 8888
#define MAX_BUFFER 1024

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[MAX_BUFFER];
    
    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind socket
    if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(1);
    }
    
    printf("UDP Calculator Server running on port %d...\n", PORT);
    printf("Waiting for client requests...\n\n");
    
    int packet_count = 0;
    
    while(1) {
        // Clear buffer
        memset(buffer, 0, MAX_BUFFER);
        
        // Receive data from client
        int recv_len = recvfrom(sockfd, buffer, MAX_BUFFER, 0, 
                               (struct sockaddr*)&client_addr, &addr_len);
        
        if(recv_len < 0) {
            perror("Receive failed");
            continue;
        }
        
        packet_count++;
        printf("Packet #%d received from %s:%d\n", 
               packet_count, 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));
        printf("Request: %s\n", buffer);
        
        // Parse and calculate
        double num1, num2, result;
        char op[10];
        char response[MAX_BUFFER];
        
        int parsed = sscanf(buffer, "%s %lf %lf", op, &num1, &num2);
        
        if(parsed >= 2) {
            // Perform operation
            if(strcmp(op, "sin") == 0) {
                result = sin(num1 * M_PI / 180.0); // Convert to radians
                snprintf(response, MAX_BUFFER, "sin(%.2f) = %.4f", num1, result);
            }
            else if(strcmp(op, "cos") == 0) {
                result = cos(num1 * M_PI / 180.0);
                snprintf(response, MAX_BUFFER, "cos(%.2f) = %.4f", num1, result);
            }
            else if(strcmp(op, "tan") == 0) {
                result = tan(num1 * M_PI / 180.0);
                snprintf(response, MAX_BUFFER, "tan(%.2f) = %.4f", num1, result);
            }
            else if(strcmp(op, "log") == 0) {
                result = log(num1);
                snprintf(response, MAX_BUFFER, "log(%.2f) = %.4f", num1, result);
            }
            else if(strcmp(op, "sqrt") == 0) {
                result = sqrt(num1);
                snprintf(response, MAX_BUFFER, "sqrt(%.2f) = %.4f", num1, result);
            }
            else if(strcmp(op, "inv") == 0) {
                result = 1.0 / num1;
                snprintf(response, MAX_BUFFER, "1/%.2f = %.4f", num1, result);
            }
            else if(strcmp(op, "+") == 0 && parsed == 3) {
                result = num1 + num2;
                snprintf(response, MAX_BUFFER, "%.2f + %.2f = %.4f", num1, num2, result);
            }
            else if(strcmp(op, "-") == 0 && parsed == 3) {
                result = num1 - num2;
                snprintf(response, MAX_BUFFER, "%.2f - %.2f = %.4f", num1, num2, result);
            }
            else if(strcmp(op, "*") == 0 && parsed == 3) {
                result = num1 * num2;
                snprintf(response, MAX_BUFFER, "%.2f * %.2f = %.4f", num1, num2, result);
            }
            else if(strcmp(op, "/") == 0 && parsed == 3) {
                if(num2 != 0) {
                    result = num1 / num2;
                    snprintf(response, MAX_BUFFER, "%.2f / %.2f = %.4f", num1, num2, result);
                } else {
                    snprintf(response, MAX_BUFFER, "Error: Division by zero");
                }
            }
            else if(strcmp(op, "pow") == 0 && parsed == 3) {
                result = pow(num1, num2);
                snprintf(response, MAX_BUFFER, "%.2f ^ %.2f = %.4f", num1, num2, result);
            }
            else {
                snprintf(response, MAX_BUFFER, "Error: Invalid operation or parameters");
            }
        } else {
            snprintf(response, MAX_BUFFER, "Error: Invalid format. Use: operation num1 [num2]");
        }
        
        // Send response back to client
        if(sendto(sockfd, response, strlen(response), 0, 
                 (struct sockaddr*)&client_addr, addr_len) < 0) {
            perror("Send failed");
        } else {
            printf("Response: %s\n\n", response);
        }
    }
    
    close(sockfd);
    return 0;
}
