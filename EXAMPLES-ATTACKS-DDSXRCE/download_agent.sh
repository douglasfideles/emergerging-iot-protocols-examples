#!/bin/bash

echo "=== Baixando Agent Pré-compilado ==="
echo ""

# Criar diretório
mkdir -p agent
cd agent

# Detectar arquitetura
ARCH=$(uname -m)
OS=$(uname -s)

echo "[*] Detectado: $OS $ARCH"

# URLs de releases do eProsima
if [ "$OS" == "Linux" ] && [ "$ARCH" == "x86_64" ]; then
    echo "[*] Baixando agent para Linux x86_64..."
    wget https://github.com/eProsima/Micro-XRCE-DDS-Agent/releases/download/v2.4.2/MicroXRCEAgent-Linux-x64.tar.gz
    tar -xzf MicroXRCEAgent-Linux-x64.tar.gz
    chmod +x MicroXRCEAgent
    echo "[✓] Agent instalado em: ./agent/MicroXRCEAgent"
    echo ""
    echo "Para executar:"
    echo "  ./agent/MicroXRCEAgent udp4 -p 2018"
else
    echo "[!] Arquitetura não suportada para download pré-compilado"
    echo "Use Docker ou compile manualmente com setup_agent.sh"
fi
