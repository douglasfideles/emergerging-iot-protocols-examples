#!/bin/bash

echo "=== Installing Zenoh Router for Linux ==="
echo ""

# Detect OS
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
else
    echo "[ERROR] Cannot detect OS"
    exit 1
fi

# Install build essentials
echo "[*] Installing build essentials..."
if [ "$OS" = "ubuntu" ] || [ "$OS" = "debian" ]; then
    sudo apt update
    sudo apt install -y build-essential gcc make wget unzip
elif [ "$OS" = "fedora" ] || [ "$OS" = "rhel" ] || [ "$OS" = "centos" ]; then
    sudo dnf install -y gcc make wget unzip
elif [ "$OS" = "arch" ]; then
    sudo pacman -S --noconfirm gcc make wget unzip
else
    echo "[WARNING] Unknown OS: $OS, trying generic install..."
    sudo apt update && sudo apt install -y build-essential gcc make wget unzip
fi

# Install Zenoh router (for testing Zenoh-Pico attacks)
echo "[*] Installing Zenoh router..."
ZENOH_VERSION="1.7.2"

# Try Debian package first (compatible with older GLIBC)
ZENOH_URL_DEBIAN="https://github.com/eclipse-zenoh/zenoh/releases/download/${ZENOH_VERSION}/zenoh-${ZENOH_VERSION}-x86_64-unknown-linux-gnu-debian.zip"

cd /tmp
rm -rf zenoh-*

echo "[*] Trying Debian package (compatible with Ubuntu 18.04)..."
echo "[*] URL: $ZENOH_URL_DEBIAN"

if wget "$ZENOH_URL_DEBIAN" -O zenoh.zip 2>&1; then
    echo "[*] Extracting Zenoh router..."
    unzip -q zenoh.zip
    
    echo "[*] Testing binary compatibility..."
    if ./zenohd --version &> /dev/null; then
        echo "[*] Binary works! Installing to /usr/local/bin..."
        sudo cp zenohd /usr/local/bin/
        sudo chmod +x /usr/local/bin/zenohd
        
        # Also copy other Zenoh tools
        if [ -f "z_pub" ]; then
            sudo cp z_* /usr/local/bin/ 2>/dev/null
        fi
        
        rm -rf zenoh.zip zenohd z_*
        ZENOH_INSTALLED=1
        
        echo "[✓] Zenoh router binary installed"
    else
        echo "[!] Binary incompatible with this system (GLIBC too old)"
        rm -rf zenoh.zip zenohd z_*
        ZENOH_INSTALLED=0
    fi
else
    echo "[!] Debian package download failed, trying cargo installation..."
    echo ""
    
    # Check if cargo is installed
    if ! command -v cargo &> /dev/null; then
        echo "[*] Installing Rust and Cargo..."
        curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
        source "$HOME/.cargo/env"
    fi
    
    if command -v cargo &> /dev/null; then
        echo "[*] Building Zenoh from source (this may take 5-10 minutes)..."
        cargo install zenoh --version ${ZENOH_VERSION}
        
        if [ $? -eq 0 ]; then
            # Cargo installs to ~/.cargo/bin, link to /usr/local/bin
            if [ -f "$HOME/.cargo/bin/zenohd" ]; then
                sudo ln -sf "$HOME/.cargo/bin/zenohd" /usr/local/bin/zenohd
                ZENOH_INSTALLED=1
                echo "[✓] Zenoh router compiled and installed"
            else
                ZENOH_INSTALLED=0
            fi
        else
            ZENOH_INSTALLED=0
        fi
    else
        ZENOH_INSTALLED=0
    fi
fi

if [ "$ZENOH_INSTALLED" -eq 0 ]; then
    echo ""
    echo "[WARNING] Automatic installation failed"
    echo ""
    echo "Your Ubuntu 18.04 has GLIBC 2.27, but Zenoh requires 2.28+"
    echo ""
    echo "Manual installation options:"
    echo ""
    echo "Method 1 - Download binary manually:"
    echo "  wget https://github.com/eclipse-zenoh/zenoh/releases/download/${ZENOH_VERSION}/zenoh-${ZENOH_VERSION}-x86_64-unknown-linux-gnu-standalone.zip"
    echo "  unzip zenoh-${ZENOH_VERSION}-x86_64-unknown-linux-gnu-standalone.zip"
    echo "  sudo cp zenohd /usr/local/bin/"
    echo ""
    echo "Method 2 - Using cargo (requires Rust):"
    echo "  curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh"
    echo "  source \$HOME/.cargo/env"
    echo "  cargo install zenoh"
    echo ""
    echo "Method 3 - Check other releases:"
    echo "  https://github.com/eclipse-zenoh/zenoh/releases"
    echo ""
    
    ZENOH_INSTALLED=0
fi

# Verify installation
echo ""
echo "=== Setup Complete ==="
echo ""

if command -v zenohd &> /dev/null; then
    echo "[✓] Zenoh router installed successfully"
    zenohd --version
    echo ""
else
    echo "[!] Zenoh router not installed yet"
    echo "    Install manually before running tests"
    echo ""
fi

echo "Next steps:"
echo "  1. Compile attacks: ./compile_all.sh"
if ! command -v zenohd &> /dev/null; then
    echo "  2. Install Zenoh (see methods above)"
    echo "  3. Run Zenoh: zenohd (in another terminal)"
    echo "  4. Test attacks: ./run_tests.sh"
else
    echo "  2. Run Zenoh: zenohd (in another terminal)"
    echo "  3. Test attacks: ./run_tests.sh"
fi
