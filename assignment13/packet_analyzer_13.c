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

void packet_handler(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {
    static int packet_count = 0;
    packet_count++;
    
    printf("\n=== Packet #%d ===\n", packet_count);
    printf("Timestamp: %s", ctime((const time_t*)&header->ts.tv_sec));
    printf("Packet Size: %d bytes\n", header->len);
    printf("Captured Size: %d bytes\n", header->caplen);
    
    // Ethernet header
    struct ether_header *eth_header = (struct ether_header *)packet;
    printf("\n--- Layer 2: Ethernet ---\n");
    printf("Source MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           eth_header->ether_shost[0], eth_header->ether_shost[1],
           eth_header->ether_shost[2], eth_header->ether_shost[3],
           eth_header->ether_shost[4], eth_header->ether_shost[5]);
    printf("Dest MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           eth_header->ether_dhost[0], eth_header->ether_dhost[1],
           eth_header->ether_dhost[2], eth_header->ether_dhost[3],
           eth_header->ether_dhost[4], eth_header->ether_dhost[5]);
    printf("EtherType: 0x%04x\n", ntohs(eth_header->ether_type));
    
    // Check for IP packets
    if (ntohs(eth_header->ether_type) == 0x0800) {
        struct iphdr *ip_header = (struct iphdr *)(packet + sizeof(struct ether_header));
        
        printf("\n--- Layer 3: IP ---\n");
        printf("Source IP: %s\n", inet_ntoa(*(struct in_addr*)&ip_header->saddr));
        printf("Dest IP: %s\n", inet_ntoa(*(struct in_addr*)&ip_header->daddr));
        printf("Protocol: %d", ip_header->protocol);
        
        // Check protocol type
        if (ip_header->protocol == 1) {
            printf(" (ICMP)\n");
            struct icmphdr *icmp_header = (struct icmphdr *)(packet + sizeof(struct ether_header) + (ip_header->ihl * 4));
            printf("ICMP Type: %d", icmp_header->type);
            if (icmp_header->type == 8) printf(" (Echo Request)");
            else if (icmp_header->type == 0) printf(" (Echo Reply)");
            printf("\nICMP Code: %d\n", icmp_header->code);
        }
        else if (ip_header->protocol == 6) {
            printf(" (TCP)\n");
            struct tcphdr *tcp_header = (struct tcphdr *)(packet + sizeof(struct ether_header) + (ip_header->ihl * 4));
            printf("Source Port: %d\n", ntohs(tcp_header->source));
            printf("Dest Port: %d\n", ntohs(tcp_header->dest));
            printf("Flags: ");
            if (tcp_header->syn) printf("SYN ");
            if (tcp_header->ack) printf("ACK ");
            if (tcp_header->fin) printf("FIN ");
            printf("\n");
        }
        else if (ip_header->protocol == 17) {
            printf(" (UDP)\n");
            struct udphdr *udp_header = (struct udphdr *)(packet + sizeof(struct ether_header) + (ip_header->ihl * 4));
            printf("Source Port: %d\n", ntohs(udp_header->source));
            printf("Dest Port: %d\n", ntohs(udp_header->dest));
        }
        else {
            printf("\n");
        }
    }
    
    printf("========================================\n");
}

int main(int argc, char *argv[]) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;
    
    if (argc != 2) {
        printf("Usage: %s <pcap_file>\n", argv[0]);
        printf("Example: %s root_capture.pcap\n", argv[0]);
        exit(1);
    }
    
    printf("========================================\n");
    printf("Packet Analyzer for Binary Tree Topology\n");
    printf("========================================\n\n");
    
    // Open pcap file
    handle = pcap_open_offline(argv[1], errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Couldn't open file %s: %s\n", argv[1], errbuf);
        exit(1);
    }
    
    printf("Analyzing packet capture: %s\n", argv[1]);
    printf("Extracting headers and protocols...\n\n");
    
    // Read and process packets
    pcap_loop(handle, 0, packet_handler, NULL);
    
    pcap_close(handle);
    
    printf("\n========================================\n");
    printf("Analysis Complete\n");
    printf("========================================\n");
    
    return 0;
}
