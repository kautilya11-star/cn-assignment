#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <time.h>
#include <net/ethernet.h>

typedef struct {
    char time_str[20];
    char src_ip[16];
    char dst_ip[16];
    char protocol[10];
    int packet_num;
    double timestamp;
} PacketInfo;

int main(int argc, char *argv[]) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;
    const u_char *packet;
    struct pcap_pkthdr header;
    struct tm *timeinfo;
    int packet_count = 0;
    
    PacketInfo packets[1000];
    
    if (argc != 2) {
        printf("Usage: %s <pcap_file>\n", argv[0]);
        exit(1);
    }
    
    // Open pcap file
    handle = pcap_open_offline(argv[1], errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Couldn't open file %s: %s\n", argv[1], errbuf);
        exit(1);
    }
    
    printf("========================================\n");
    printf("TIME DIAGRAM for PING Operation\n");
    printf("========================================\n\n");
    
    printf("TIME      Source       Destination  Protocol  Packet\n");
    printf("----------------------------------------------------\n");
    
    // Read packets
    while ((packet = pcap_next(handle, &header)) != NULL) {
        struct ether_header *eth = (struct ether_header *)packet;
        
        if (ntohs(eth->ether_type) == 0x0800) {
            struct iphdr *ip = (struct iphdr *)(packet + sizeof(struct ether_header));
            
            // Get timestamp
            timeinfo = localtime(&header.ts.tv_sec);
            strftime(packets[packet_count].time_str, 20, "%H:%M:%S", timeinfo);
            
            // Get IP addresses
            strcpy(packets[packet_count].src_ip, inet_ntoa(*(struct in_addr*)&ip->saddr));
            strcpy(packets[packet_count].dst_ip, inet_ntoa(*(struct in_addr*)&ip->daddr));
            
            // Get protocol
            if (ip->protocol == 1) {
                struct icmphdr *icmp = (struct icmphdr *)(packet + sizeof(struct ether_header) + (ip->ihl * 4));
                if (icmp->type == 8) strcpy(packets[packet_count].protocol, "ICMP_REQ");
                else if (icmp->type == 0) strcpy(packets[packet_count].protocol, "ICMP_REP");
                else strcpy(packets[packet_count].protocol, "ICMP");
            }
            else if (ip->protocol == 6) strcpy(packets[packet_count].protocol, "TCP");
            else if (ip->protocol == 17) strcpy(packets[packet_count].protocol, "UDP");
            else strcpy(packets[packet_count].protocol, "OTHER");
            
            packets[packet_count].packet_num = packet_count + 1;
            packets[packet_count].timestamp = header.ts.tv_sec + header.ts.tv_usec / 1000000.0;
            
            // Print in table format
            printf("%s  %-12s %-12s %-9s %d\n",
                   packets[packet_count].time_str,
                   packets[packet_count].src_ip,
                   packets[packet_count].dst_ip,
                   packets[packet_count].protocol,
                   packets[packet_count].packet_num);
            
            packet_count++;
            if (packet_count >= 1000) break;
        }
    }
    
    printf("\n========================================\n");
    printf("GRAPHICAL TIME DIAGRAM\n");
    printf("========================================\n\n");
    
    // Create simple ASCII diagram
    for (int i = 0; i < packet_count && i < 10; i++) {
        printf("Time %s: ", packets[i].time_str);
        
        if (strcmp(packets[i].protocol, "ICMP_REQ") == 0) {
            printf("%s ---ICMP Echo Request---> %s\n", packets[i].src_ip, packets[i].dst_ip);
        }
        else if (strcmp(packets[i].protocol, "ICMP_REP") == 0) {
            printf("%s <--ICMP Echo Reply---- %s\n", packets[i].src_ip, packets[i].dst_ip);
        }
        else if (strcmp(packets[i].protocol, "TCP") == 0) {
            printf("%s ---TCP Packet---------> %s\n", packets[i].src_ip, packets[i].dst_ip);
        }
    }
    
    printf("\n========================================\n");
    printf("PROTOCOL SUMMARY\n");
    printf("========================================\n\n");
    
    printf("Layer 2 (Data Link) Protocols:\n");
    printf("  - Ethernet (0x0800)\n");
    printf("  - ARP (0x0806) - if present\n\n");
    
    printf("Layer 3 (Network) Protocols:\n");
    printf("  - IPv4 (0x0800)\n");
    printf("  - ICMP (Protocol 1)\n");
    printf("  - TCP (Protocol 6) - if present\n");
    printf("  - UDP (Protocol 17) - if present\n\n");
    
    printf("Layer 4 (Transport) Protocols:\n");
    printf("  - ICMP (for PING)\n");
    printf("  - TCP Ports (if TCP traffic)\n");
    printf("  - UDP Ports (if UDP traffic)\n");
    
    pcap_close(handle);
    return 0;
}
