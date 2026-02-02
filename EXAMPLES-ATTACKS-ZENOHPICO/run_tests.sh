#!/bin/bash

echo "=== Zenoh-Pico Attack Tests ==="
echo ""

# Check if binaries exist
if [ ! -d "./bin" ] || [ -z "$(ls -A ./bin 2>/dev/null)" ]; then
    echo "[ERROR] Binaries not found!"
    echo "Run first: ./compile_all.sh"
    exit 1
fi

ROUTER_IP="${1:-127.0.0.1}"
ROUTER_PORT="${2:-7447}"

echo "Target: $ROUTER_IP:$ROUTER_PORT"
echo ""

# Check if Zenoh is running
if ! timeout 1 bash -c "echo > /dev/tcp/$ROUTER_IP/$ROUTER_PORT" 2>/dev/null; then
    echo "[WARNING] Cannot connect to Zenoh router"
    echo "Make sure zenohd is running in another terminal"
    echo ""
fi

echo "=== Test Menu ==="
echo ""
echo "1. DoS KeepAlive Flood"
echo "2. Malformed Init"
echo "3. Fragment Bomb"
echo "4. Protocol Fuzzer"
echo "5. Memory Exhaustion"
echo "6. Slowloris"
echo "7. QoS Inversion"
echo "8. Liveliness Flood"
echo "9. Run ALL (quick test - 2s each)"
echo "0. Exit"
echo ""
read -p "Choose: " choice

run_attack() {
    local binary=$1
    local name=$2
    shift 2
    local args=("$@")
    
    if [ ! -f "$binary" ]; then
        echo "[ERROR] $binary not found"
        return 1
    fi
    
    echo ""
    echo "[*] Running: $name"
    echo "[*] Command: $binary ${args[@]}"
    echo "[*] Press Ctrl+C to stop"
    echo ""
    
    "$binary" "${args[@]}"
}

case $choice in
    1)
        run_attack "./bin/attack1_keepalive" "DoS KeepAlive Flood" "$ROUTER_IP" "$ROUTER_PORT" 5
        ;;
    2)
        run_attack "./bin/attack2_malformed" "Malformed Init" "$ROUTER_IP" "$ROUTER_PORT" 5
        ;;
    3)
        run_attack "./bin/attack3_fragbomb" "Fragment Bomb" "$ROUTER_IP" "$ROUTER_PORT" 3
        ;;
    4)
        run_attack "./bin/attack5_fuzzer" "Protocol Fuzzer" "$ROUTER_IP" "$ROUTER_PORT" 500
        ;;
    5)
        run_attack "./bin/attack9_memexhaust" "Memory Exhaustion" "$ROUTER_IP" "$ROUTER_PORT" 2
        ;;
    6)
        run_attack "./bin/attack10_slowloris" "Slowloris" "$ROUTER_IP" "$ROUTER_PORT" 100
        ;;
    7)
        run_attack "./bin/attack14_qos" "QoS Inversion" "$ROUTER_IP" "$ROUTER_PORT" 5
        ;;
    8)
        run_attack "./bin/attack17_liveliness" "Liveliness Flood" "$ROUTER_IP" "$ROUTER_PORT" 5
        ;;
    9)
        echo "[*] Running ALL attacks (quick test - 2s each)..."
        echo ""
        
        for attack in "attack1_keepalive:KeepAlive Flood" \
                      "attack2_malformed:Malformed Init" \
                      "attack3_fragbomb:Fragment Bomb" \
                      "attack5_fuzzer:Protocol Fuzzer" \
                      "attack9_memexhaust:Memory Exhaustion" \
                      "attack10_slowloris:Slowloris" \
                      "attack14_qos:QoS Inversion" \
                      "attack17_liveliness:Liveliness Flood"; do
            
            IFS=':' read -r binary name <<< "$attack"
            
            if [ -f "./bin/$binary" ]; then
                echo "[*] Testing: $name"
                timeout 2 "./bin/$binary" "$ROUTER_IP" "$ROUTER_PORT" 5 > /dev/null 2>&1
                if [ $? -eq 0 ] || [ $? -eq 124 ]; then
                    echo "    ✓ PASS"
                else
                    echo "    ✗ FAIL"
                fi
            else
                echo "[*] Testing: $name"
                echo "    ⊘ SKIP (not compiled)"
            fi
        done
        
        echo ""
        echo "[✓] All tests completed!"
        ;;
    0)
        exit 0
        ;;
    *)
        echo "[ERROR] Invalid option!"
        exit 1
        ;;
esac
