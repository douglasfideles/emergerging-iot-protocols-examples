#!/bin/bash

echo "=== Compiling DDS Attack Tools ==="
echo ""

# Check for required libraries
if ! pkg-config --exists microxrcedds_client; then
    echo "[ERROR] microxrcedds_client not found!"
    echo "Install with: sudo apt install libmicroxrcedds-client-dev"
    exit 1
fi

ATTACKS=(
    "attack_session_hijack"
    "attack_entity_flood"
    "attack_malformed_inject"
    "attack_time_desync"
    "attack_discovery_poison"
    "attack_request_race"
    "attack_fragment_abuse"
)

for attack in "${ATTACKS[@]}"; do
    echo "[*] Compiling $attack..."
    gcc ${attack}.c -o $attack \
        -lmicroxrcedds_client \
        -lmicrocdr \
        -lpthread \
        -O2 \
        -Wall
    
    if [ $? -eq 0 ]; then
        echo "[+] $attack compiled successfully"
    else
        echo "[-] Failed to compile $attack"
    fi
    echo ""
done

echo "=== Compilation Complete ==="
ls -lh attack_*