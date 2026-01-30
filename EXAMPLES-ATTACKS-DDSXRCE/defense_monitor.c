// Basic monitoring tool to detect attacks
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <time.h>

#define MAX_SESSION_KEYS 1000
#define ALERT_THRESHOLD 100

typedef struct {
    uint32_t key;
    int count;
    time_t first_seen;
} session_tracker_t;

session_tracker_t sessions[MAX_SESSION_KEYS];
int session_count = 0;

void packet_handler(u_char* args, const struct pcap_pkthdr* header, const u_char* packet) {
    struct ip* ip_header = (struct ip*)(packet + 14); // Skip Ethernet header
    
    if (ip_header->ip_p == IPPROTO_UDP) {
        struct udphdr* udp_header = (struct udphdr*)(packet + 14 + (ip_header->ip_hl * 4));
        
        // Check if DDS port (common: 7400, 2018, etc.)
        int dest_port = ntohs(udp_header->uh_dport);
        
        if (dest_port == 2018 || dest_port == 7400) {
            // Analyze payload for session key patterns
            const u_char* payload = packet + 14 + (ip_header->ip_hl * 4) + 8;
            int payload_len = header->len - 14 - (ip_header->ip_hl * 4) - 8;
            
            if (payload_len > 4) {
                uint32_t potential_key = *(uint32_t*)(payload + 4);
                
                // Track session
                int found = 0;
                for (int i = 0; i < session_count; i++) {
                    if (sessions[i].key == potential_key) {
                        sessions[i].count++;
                        found = 1;
                        
                        if (sessions[i].count > ALERT_THRESHOLD) {
                            printf("[ALERT] Potential attack! Session key 0x%08X seen %d times\n",
                                   potential_key, sessions[i].count);
                        }
                        break;
                    }
                }
                
                if (!found && session_count < MAX_SESSION_KEYS) {
                    sessions[session_count].key = potential_key;
                    sessions[session_count].count = 1;
                    sessions[session_count].first_seen = time(NULL);
                    session_count++;
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <interface>\n", argv[0]);
        return 1;
    }
    
    char* dev = argv[1];
    char errbuf[PCAP_ERRBUF_SIZE];
    
    printf("=== DDS Attack Detection Monitor ===\n");
    printf("Monitoring interface: %s\n\n", dev);
    
    pcap_t* handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Error opening device %s: %s\n", dev, errbuf);
        return 1;
    }
    
    printf("[*] Starting packet capture...\n");
    pcap_loop(handle, 0, packet_handler, NULL);
    
    pcap_close(handle);
    return 0;
}