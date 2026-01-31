#!/bin/bash

echo "=============================================="
echo "  Micro XRCE-DDS Agent - Instalação"
echo "=============================================="
echo ""
echo "AVISO: A instalação completa requer ~500MB+ e"
echo "demora 10-20 minutos."
echo ""
echo "ALTERNATIVAS MAIS RÁPIDAS:"
echo ""
echo "1. Docker (Recomendado):"
echo "   ./run_agent_docker.sh"
echo ""
echo "2. Binário Pré-compilado:"
echo "   ./download_agent.sh"
echo ""
echo "3. Agent Remoto:"
echo "   Use um agent já rodando em outro servidor"
echo ""
read -p "Deseja realmente instalar do zero? (y/n) " confirm

if [ "$confirm" != "y" ]; then
    echo "Instalação cancelada. Use uma das alternativas acima."
    exit 0
fi

echo ""
echo "[*] Instalando dependências (pode demorar)..."
echo ""

sudo apt update
sudo apt install -y default-jre gradle

# Fast-DDS dependencies
cd /tmp
rm -rf foonathan_memory_vendor
git clone https://github.com/eProsima/foonathan_memory_vendor.git
cd foonathan_memory_vendor
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_SHARED_LIBS=ON
sudo cmake --build . --target install

cd /tmp
rm -rf Fast-CDR
git clone https://github.com/eProsima/Fast-CDR.git
cd Fast-CDR
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
sudo cmake --build . --target install

cd /tmp
rm -rf Fast-DDS
git clone https://github.com/eProsima/Fast-DDS.git
cd Fast-DDS
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_PREFIX_PATH=/usr/local
sudo cmake --build . --target install
sudo ldconfig

# Agent
cd /tmp
rm -rf Micro-XRCE-DDS-Agent
git clone https://github.com/eProsima/Micro-XRCE-DDS-Agent.git
cd Micro-XRCE-DDS-Agent
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make -j$(nproc)
sudo make install
sudo ldconfig

echo ""
echo "=== Instalação Completa ==="
echo ""
echo "Para rodar: MicroXRCEAgent udp4 -p 2018"
