#!/bin/bash

echo "======================================================================"
echo "  Testing CoAP Attacks - GT-IoTEdu"
echo "======================================================================"
echo ""

TARGET="172.17.0.3"  # Default Docker bridge network IP
PORT="5683"

# Check if server is running
if ! docker ps | grep -q coap-server; then
    echo "[!] CoAP server is not running. Starting it..."
    ./setup_server.sh
    sleep 5
fi

# Get actual server IP
ACTUAL_IP=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' coap-server)
if [ ! -z "$ACTUAL_IP" ]; then
    TARGET="$ACTUAL_IP"
fi

echo "[*] Testing attacks against server: $TARGET:$PORT"
echo ""

# Test 1: Resource Exhaustion
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 1: Resource Discovery & Exhaustion"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
timeout 10 docker run --rm --network bridge iotedu-coap-attacks:latest attack-coap-resource-exhaustion_attack.py $TARGET $PORT 2>&1 | grep -v "Connection loss" | head -20
echo ""
sleep 2

# Test 2: Token Collision
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 2: Token Collision Attack"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
timeout 10 docker run --rm --network bridge iotedu-coap-attacks:latest attack-coap-token-collision_attack.py $TARGET $PORT 2>&1 | grep -v "Connection loss" | head -20
echo ""
sleep 2

# Test 3: Block Fragmentation
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 3: Block-Wise Transfer Fragmentation"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
timeout 10 docker run --rm --network bridge iotedu-coap-attacks:latest attack-coap-block-fragmentation_attack.py $TARGET $PORT 2>&1 | grep -v "Connection loss" | head -20
echo ""
sleep 2

# Test 4: Observe Amplification
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 4: Observe Amplification"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
timeout 10 docker run --rm --network bridge iotedu-coap-attacks:latest attack-coap-observe-amplification_attack.py $TARGET $PORT 2>&1 | grep -v "Connection loss" | head -20
echo ""

echo "======================================================================"
echo "  Tests Completed!"
echo "======================================================================"
echo ""
echo "To monitor server logs:"
echo "  docker logs coap-server"
echo ""
echo "To stop server:"
echo "  docker stop coap-server"
echo ""
