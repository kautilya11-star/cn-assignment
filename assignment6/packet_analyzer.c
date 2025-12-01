#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/ether.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 65536

int main() {
    int raw_socket;
    struct sockaddr saddr;
    int saddr_size = sizeof(saddr);
    unsigned char buffer[BUFFER_SIZE];
    
    // Create raw socket
    raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(raw_socket < 0) {
        perror("Socket Error");
        return 1;
    }
    
    printf("Starting TCP Packet Analyzer...\n");
    printf("Press Ctrl+C to stop\n\n");
    
    int packet_count = 0;
    
    while(1) {
        // Receive packet
        int packet_size = recvfrom(raw_socket, buffer, BUFFER_SIZE, 0, &saddr, (socklen_t*)&saddr_size);
        if(packet_size < 0) {
            perror("Packet Receive Error");
            close(raw_socket);
            return 1;
        }
        
        packet_count++;
        printf("\n=== Packet #%d ===\n", packet_count);
        printf("Packet Size: %d bytes\n", packet_size);
        
        // Extract Ethernet header
        struct ethhdr *eth = (struct ethhdr *)buffer;
        printf("\n--- Layer 2: Ethernet Header ---\n");
        printf("Source MAC: %.2X:%.2X:%.2X:%.2X:%.2X:%.2X\n",
               eth->h_source[0], eth->h_source[1], eth->h_source[2],
               eth->h_source[3], eth->h_source[4], eth->h_source[5]);
        printf("Dest MAC: %.2X:%.2X:%.2X:%.2X:%.2X:%.2X\n",
               eth->h_dest[0], eth->h_dest[1], eth->h_dest[2],
               eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
        printf("Protocol: 0x%04X\n", ntohs(eth->h_proto));
        
        // Check if it's IP packet (0x0800)
        if(ntohs(eth->h_proto) == 0x0800) {
            // Extract IP header
            struct iphdr *ip = (struct iphdr*)(buffer + sizeof(struct ethhdr));
            printf("\n--- Layer 3: IP Header ---\n");
            printf("Source IP: %s\n", inet_ntoa(*(struct in_addr*)&ip->saddr));
            printf("Dest IP: %s\n", inet_ntoa(*(struct in_addr*)&ip->daddr));
            printf("Protocol: %d", ip->protocol);
            
            // Check if it's TCP (protocol = 6)
            if(ip->protocol == 6) {
                printf(" (TCP)\n");
                
                // Extract TCP header
                struct tcphdr *tcp = (struct tcphdr*)(buffer + sizeof(struct ethhdr) + (ip->ihl * 4));
                printf("\n--- Layer 4: TCP Header ---\n");
                printf("Source Port: %d\n", ntohs(tcp->source));
                printf("Dest Port: %d\n", ntohs(tcp->dest));
                printf("Sequence: %u\n", ntohl(tcp->seq));
                printf("Ack Number: %u\n", ntohl(tcp->ack_seq));
                printf("Flags: ");
                if(tcp->syn) printf("SYN ");
                if(tcp->ack) printf("ACK ");
                if(tcp->fin) printf("FIN ");
                if(tcp->rst) printf("RST ");
                if(tcp->psh) printf("PSH ");
                if(tcp->urg) printf("URG ");
                printf("\n");
                
                // Calculate data position and size
                int ip_header_len = ip->ihl * 4;
                int tcp_header_len = tcp->doff * 4;
                int total_headers_size = sizeof(struct ethhdr) + ip_header_len + tcp_header_len;
                int data_size = packet_size - total_headers_size;
                
                if(data_size > 0) {
                    printf("\n--- TCP Data (%d bytes) ---\n", data_size);
                    printf("Data (first 50 bytes): ");
                    for(int i = 0; i < data_size && i < 50; i++) {
                        printf("%02X ", buffer[total_headers_size + i]);
                    }
                    printf("\n");
                    
                    // Print as ASCII if printable
                    printf("ASCII: ");
                    for(int i = 0; i < data_size && i < 50; i++) {
                        unsigned char c = buffer[total_headers_size + i];
                        if(c >= 32 && c <= 126) {
                            printf("%c", c);
                        } else {
                            printf(".");
                        }
                    }
                    printf("\n");
                }
            } else {
                printf("\n");
            }
        }
        
        printf("==================================================\n");
        fflush(stdout);
        
        // Slow down output
        sleep(1);
    }
    
    close(raw_socket);
    return 0;
}
