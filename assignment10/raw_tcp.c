#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

// Function to calculate checksum
unsigned short checksum(unsigned short *buf, int nwords) {
    unsigned long sum;
    for (sum = 0; nwords > 0; nwords--) {
        sum += *buf++;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

int main(int argc, char *argv[]) {
    int raw_socket;
    char buffer[4096];
    struct iphdr *ip = (struct iphdr *)buffer;
    struct tcphdr *tcp = (struct tcphdr *)(buffer + sizeof(struct iphdr));
    struct sockaddr_in dest_addr;
    
    if (argc != 3) {
        printf("Usage: %s <source_ip> <dest_ip>\n", argv[0]);
        printf("Example: %s 192.168.1.100 192.168.1.1\n", argv[0]);
        exit(1);
    }
    
    // Create raw socket
    raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (raw_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // Enable IP_HDRINCL to manually set IP header
    int one = 1;
    const int *val = &one;
    if (setsockopt(raw_socket, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0) {
        perror("Setsockopt failed");
        close(raw_socket);
        exit(1);
    }
    
    // Clear buffer
    memset(buffer, 0, sizeof(buffer));
    
    // Fill IP header
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr) + 10); // 10 bytes for roll number
    ip->id = htons(54321);
    ip->frag_off = 0;
    ip->ttl = 255;
    ip->protocol = IPPROTO_TCP;
    ip->saddr = inet_addr(argv[1]);
    ip->daddr = inet_addr(argv[2]);
    ip->check = 0;
    ip->check = checksum((unsigned short *)buffer, sizeof(struct iphdr) / 2);
    
    // Fill TCP header
    tcp->source = htons(12345);  // Source port
    tcp->dest = htons(80);       // Destination port (HTTP)
    tcp->seq = htonl(110502);    // Sequence number
    tcp->ack_seq = 0;
    tcp->doff = 5;               // TCP header length
    tcp->fin = 0;
    tcp->syn = 1;                // SYN flag set
    tcp->rst = 0;
    tcp->psh = 0;
    tcp->ack = 0;
    tcp->urg = 0;
    tcp->window = htons(5840);
    tcp->check = 0;              // Will calculate later
    tcp->urg_ptr = 0;
    
    // Add payload (your roll number)
    char *payload = buffer + sizeof(struct iphdr) + sizeof(struct tcphdr);
    char roll_number[] = "2023123456";  // Replace with your actual roll number
    strcpy(payload, roll_number);
    
    // Calculate TCP checksum (pseudo header)
    struct pseudo_header {
        unsigned int source_address;
        unsigned int dest_address;
        unsigned char placeholder;
        unsigned char protocol;
        unsigned short tcp_length;
    } psh;
    
    psh.source_address = inet_addr(argv[1]);
    psh.dest_address = inet_addr(argv[2]);
    psh.placeholder = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons(sizeof(struct tcphdr) + strlen(roll_number));
    
    // Create buffer for checksum calculation
    int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr) + strlen(roll_number);
    char *pseudogram = malloc(psize);
    
    memcpy(pseudogram, (char *)&psh, sizeof(struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header), tcp, sizeof(struct tcphdr) + strlen(roll_number));
    
    tcp->check = checksum((unsigned short *)pseudogram, psize / 2);
    free(pseudogram);
    
    // Set destination address
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(80);
    dest_addr.sin_addr.s_addr = inet_addr(argv[2]);
    
    printf("Sending custom TCP packet...\n");
    printf("Source IP: %s\n", argv[1]);
    printf("Dest IP: %s\n", argv[2]);
    printf("Source Port: 12345\n");
    printf("Dest Port: 80\n");
    printf("Payload: %s\n", roll_number);
    printf("SYN flag: SET\n");
    
    // Send packet
    if (sendto(raw_socket, buffer, ntohs(ip->tot_len), 0, 
               (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("Send failed");
    } else {
        printf("Packet sent successfully!\n");
        printf("Check Wireshark to see the packet with roll number in payload.\n");
    }
    
    close(raw_socket);
    return 0;
}