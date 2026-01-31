#!/bin/bash

echo "=== Compilando Todos os Ataques DDS ==="
echo ""

# Verificar dependências
if [ ! -f "/usr/local/lib/libmicroxrcedds_client.a" ]; then
    echo "[ERROR] microxrcedds_client não encontrado!"
    echo "Instale com o script de instalação primeiro"
    exit 1
fi

if [ ! -f "/usr/local/microcdr-2.0.2/lib/libmicrocdr.a" ] && [ ! -f "/usr/local/lib/libmicrocdr.a" ]; then
    echo "[ERROR] microcdr não encontrado!"
    echo "Instale com o script de instalação primeiro"
    exit 1
fi

echo "[✓] Dependências encontradas"

# Criar diretórios
mkdir -p bin
mkdir -p logs

# Lista de ataques C
C_ATTACKS=(
    "attack_session_hijack"
    "attack_entity_flood"
    "attack_time_desync"
    "attack_malformed_inject"
    "attack_fragment_abuse"
    "attack_ping_flood"
)

# Ataque especial que não precisa das libs DDS
SPECIAL_ATTACKS=(
    "attack_discovery_poison"
)

# Flags de compilação para ataques DDS
DDS_CFLAGS="-Wall -Wextra -O2 -I/usr/local/include -I/usr/local/microcdr-2.0.2/include"
DDS_LIBS="-L/usr/local/lib -L/usr/local/microcdr-2.0.2/lib -lmicroxrcedds_client -lmicrocdr -lpthread"

# Flags para ataques especiais
SPECIAL_CFLAGS="-Wall -Wextra -O2"
SPECIAL_LIBS="-lpthread"

echo "Compilando ataques DDS..."
echo ""

for attack in "${C_ATTACKS[@]}"; do
    if [ -f "${attack}.c" ]; then
        echo "[*] Compilando $attack..."
        
        gcc ${attack}.c -o bin/$attack ${DDS_CFLAGS} ${DDS_LIBS}
        
        if [ $? -eq 0 ]; then
            echo "[✓] $attack compilado com sucesso"
            chmod +x bin/$attack
        else
            echo "[✗] Falha ao compilar $attack"
        fi
    else
        echo "[!] Arquivo ${attack}.c não encontrado"
    fi
    echo ""
done

echo "Compilando ataques especiais (sem dependências DDS)..."
echo ""

for attack in "${SPECIAL_ATTACKS[@]}"; do
    if [ -f "${attack}.c" ]; then
        echo "[*] Compilando $attack..."
        
        gcc ${attack}.c -o bin/$attack ${SPECIAL_CFLAGS} ${SPECIAL_LIBS}
        
        if [ $? -eq 0 ]; then
            echo "[✓] $attack compilado com sucesso"
            chmod +x bin/$attack
        else
            echo "[✗] Falha ao compilar $attack"
        fi
    else
        echo "[!] Arquivo ${attack}.c não encontrado"
    fi
    echo ""
done

echo ""
echo "=== Compilação Completa ==="
echo ""
echo "Ataques disponíveis em ./bin/:"
ls -lh bin/

echo ""
echo "Para executar:"
echo "  ./bin/attack_session_hijack <ip> <port>"
echo "  ./bin/attack_entity_flood <ip> <port>"
echo "  ./bin/attack_time_desync <ip> <port>"
echo "  ./bin/attack_malformed_inject <ip> <port>"
echo "  ./bin/attack_fragment_abuse <ip> <port>"
echo "  ./bin/attack_ping_flood <ip> <port>"
echo "  sudo ./bin/attack_discovery_poison <malicious_ip>"
