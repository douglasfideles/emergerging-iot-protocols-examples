#!/bin/bash

echo "=== Installing DDS-XRCE Dependencies for WSL ==="
echo ""

# Update package list
echo "[*] Updating package list..."
sudo apt update

# Install build essentials
echo "[*] Installing build essentials..."
sudo apt install -y build-essential cmake git pkg-config libpcap-dev

# Install Micro-CDR
echo "[*] Installing Micro-CDR..."
cd /tmp
rm -rf Micro-CDR
git clone https://github.com/eProsima/Micro-CDR.git
cd Micro-CDR
mkdir -p build && cd build
cmake ..
make -j$(nproc)
sudo make install
sudo ldconfig

# Install Micro XRCE-DDS Client
echo "[*] Installing Micro XRCE-DDS Client..."
cd /tmp
rm -rf Micro-XRCE-DDS-Client
git clone https://github.com/eProsima/Micro-XRCE-DDS-Client.git
cd Micro-XRCE-DDS-Client
mkdir -p build && cd build
cmake .. -DUCLIENT_BUILD_EXAMPLES=OFF
make -j$(nproc)
sudo make install
sudo ldconfig

echo ""
echo "=== Installation Complete ==="
echo ""
echo "You can now run: ./compile_all.sh"
