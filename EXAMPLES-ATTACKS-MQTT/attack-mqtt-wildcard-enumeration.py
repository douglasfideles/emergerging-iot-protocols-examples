#!/usr/bin/env python3
import paho.mqtt.client as mqtt
import time

discovered_topics = set()

def on_message(client, userdata, msg):
    discovered_topics.add(msg.topic)
    print(f"[📡] Discovered: {msg.topic} -> {msg.payload[:50]}")

def on_connect(client, userdata, flags, rc):
    print("[+] Connected - Starting wildcard enumeration...")
    
    # Subscribe a TODOS os tópicos
    client.subscribe("#", qos=0)  # Root wildcard
    client.subscribe("+/+/#", qos=0)
    client.subscribe("$SYS/#", qos=0)  # System topics
    
    time.sleep(15)  # Captura durante 15 segundos
    
    print(f"\n[✓] Enumeration completed - {len(discovered_topics)} topics discovered:")
    for topic in sorted(discovered_topics):
        print(f"  - {topic}")
    
    client.disconnect()

if __name__ == "__main__":
    import sys
    broker = sys.argv[1] if len(sys.argv) > 1 else "localhost"
    
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(broker, 1883, 60)
    client.loop_forever()