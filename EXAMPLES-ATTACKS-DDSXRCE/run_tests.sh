#!/bin/bash

echo "=== DDS-XRCE Attack Tests ==="
echo ""

# Verificar se os binários existem
if [ ! -d "./bin" ] || [ -z "$(ls -A ./bin 2>/dev/null)" ]; then
    echo "[ERROR] Binários não encontrados!"
    echo "Execute primeiro: ./compile_all.sh"
    exit 1
fi

AGENT_IP="127.0.0.1"
AGENT_PORT="2018"

echo "=== Menu de Testes ==="
echo ""
echo "1. Session Hijack"
echo "2. Entity Flood (alto uso de recursos!)"
echo "3. Ping Flood"
echo "4. Time Desync"
echo "5. Malformed Inject"
echo "6. Fragment Abuse"
echo "7. Discovery Poison (requer sudo)"
echo "8. Executar TODOS (sequencial)"
echo "0. Sair"
echo ""
read -p "Escolha: " choice

case $choice in
    1)
        echo "[*] Session Hijack Attack..."
        ./bin/attack_session_hijack $AGENT_IP $AGENT_PORT
        ;;
    2)
        echo "[*] Entity Flood Attack..."
        read -p "ATENÇÃO: Criará muitas entidades! Continuar? (y/n) " confirm
        [ "$confirm" == "y" ] && ./bin/attack_entity_flood $AGENT_IP $AGENT_PORT
        ;;
    3)
        echo "[*] Ping Flood Attack..."
        ./bin/attack_ping_flood $AGENT_IP $AGENT_PORT
        ;;
    4)
        echo "[*] Time Desync Attack..."
        ./bin/attack_time_desync $AGENT_IP $AGENT_PORT
        ;;
    5)
        echo "[*] Malformed Inject Attack..."
        ./bin/attack_malformed_inject $AGENT_IP $AGENT_PORT
        ;;
    6)
        echo "[*] Fragment Abuse Attack..."
        ./bin/attack_fragment_abuse $AGENT_IP $AGENT_PORT
        ;;
    7)
        echo "[*] Discovery Poison Attack..."
        read -p "IP malicioso: " malicious_ip
        sudo ./bin/attack_discovery_poison $malicious_ip
        ;;
    8)
        echo "[*] Executando TODOS os ataques..."
        read -p "Certifique-se que o agent está rodando! Continuar? (y/n) " confirm
        if [ "$confirm" == "y" ]; then
            ./bin/attack_session_hijack $AGENT_IP $AGENT_PORT && sleep 2
            ./bin/attack_ping_flood $AGENT_IP $AGENT_PORT && sleep 2
            ./bin/attack_time_desync $AGENT_IP $AGENT_PORT && sleep 2
            ./bin/attack_malformed_inject $AGENT_IP $AGENT_PORT && sleep 2
            ./bin/attack_fragment_abuse $AGENT_IP $AGENT_PORT
            echo "[✓] Todos os ataques completados!"
        fi
        ;;
    0)
        exit 0
        ;;
    *)
        echo "[ERROR] Opção inválida!"
        exit 1
        ;;
esac
