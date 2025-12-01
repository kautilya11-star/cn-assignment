#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <time.h>

// ICMP timestamp structure
struct icmp_timestamp {
    struct icmphdr hdr;
    unsigned int originate_ts;
    unsigned int receive_ts;
    unsigned int transmit_ts;
};

// Checksum function
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;
    
    for (sum = 0; len > 1; len -= 2) {
        sum += *buf++;
    }
    if (len == 1) {
        sum += *(unsigned char *)buf;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

int main(int argc, char *argv[]) {
    int raw_socket;
    struct sockaddr_in dest_addr;
    char packet[4096];
    struct icmp_timestamp *icmp_ts = (struct icmp_timestamp *)packet;
    
    if (argc != 2) {
        printf("Usage: %s <dest_ip>\n", argv[0]);
        printf("Example: %s 8.8.8.8\n", argv[0]);
        exit(1);
    }
    
    // Create raw socket
    raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (raw_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // Clear packet
    memset(packet, 0, sizeof(packet));
    
    // Fill ICMP timestamp header
    icmp_ts->hdr.type = ICMP_TIMESTAMP;      // Type 13 for timestamp request
    icmp_ts->hdr.code = 0;
    icmp_ts->hdr.un.echo.id = htons(getpid() & 0xFFFF);
    icmp_ts->hdr.un.echo.sequence = htons(1);
    
    // Get current time in milliseconds since midnight UTC
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    unsigned int timestamp = (tv.tv_sec % 86400) * 1000 + tv.tv_usec / 1000;
    
    icmp_ts->originate_ts = htonl(timestamp);
    icmp_ts->receive_ts = 0;      // To be filled by receiver
    icmp_ts->transmit_ts = 0;     // To be filled by receiver
    
    // Calculate checksum
    icmp_ts->hdr.checksum = 0;
    icmp_ts->hdr.checksum = checksum(icmp_ts, sizeof(struct icmp_timestamp));
    
    // Set destination address
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(argv[1]);
    
    printf("Sending ICMP Timestamp Request...\n");
    printf("Destination: %s\n", argv[1]);
    printf("ICMP Type: %d (Timestamp Request)\n", ICMP_TIMESTAMP);
    printf("ICMP Code: 0\n");
    printf("Originate Timestamp: %u ms\n", timestamp);
    printf("Sequence: 1\n");
    printf("ID: %d\n", getpid() & 0xFFFF);
    
    // Send packet
    if (sendto(raw_socket, packet, sizeof(struct icmp_timestamp), 0,
               (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("Send failed");
    } else {
        printf("ICMP Timestamp Request sent successfully!\n");
        printf("\nCheck Wireshark to see the packet:\n");
        printf("1. Filter: icmp\n");
        printf("2. Look for Type 13 (Timestamp Request)\n");
        printf("3. Expand ICMP section to see timestamps\n");
    }
    
    // Try to receive response (optional)
    printf("\nWaiting for response (5 seconds)...\n");
    
    fd_set readfds;
    struct timeval timeout;
    FD_ZERO(&readfds);
    FD_SET(raw_socket, &readfds);
    
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    
    if (select(raw_socket + 1, &readfds, NULL, NULL, &timeout) > 0) {
        char recv_buffer[4096];
        struct sockaddr_in recv_addr;
        socklen_t addr_len = sizeof(recv_addr);
        
        int recv_len = recvfrom(raw_socket, recv_buffer, sizeof(recv_buffer), 0,
                               (struct sockaddr *)&recv_addr, &addr_len);
        
        if (recv_len > 0) {
            struct iphdr *ip_hdr = (struct iphdr *)recv_buffer;
            struct icmp_timestamp *recv_icmp = (struct icmp_timestamp *)(recv_buffer + (ip_hdr->ihl * 4));
            
            if (recv_icmp->hdr.type == ICMP_TIMESTAMPREPLY) {
                printf("\nReceived ICMP Timestamp Reply!\n");
                printf("From: %s\n", inet_ntoa(recv_addr.sin_addr));
                printf("Originate TS: %u\n", ntohl(recv_icmp->originate_ts));
                printf("Receive TS: %u\n", ntohl(recv_icmp->receive_ts));
                printf("Transmit TS: %u\n", ntohl(recv_icmp->transmit_ts));
            }
        }
    } else {
        printf("No response received (this is normal - many hosts don't reply to timestamp requests)\n");
    }
    
    close(raw_socket);
    return 0;
}