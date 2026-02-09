#!/bin/bash

echo "======================================================================"
echo "  Setting up CoAP Server (Californium) - GT-IoTEdu"
echo "======================================================================"

# Check if coap-server container already exists
if [ "$(docker ps -aq -f name=coap-server)" ]; then
    echo "[*] Stopping existing coap-server container..."
    docker stop coap-server 2>/dev/null
    docker rm coap-server 2>/dev/null
fi

echo ""
echo "[*] Starting CoAP server (aiocoap-based)..."
echo "[*] Port: 5683 (CoAP)"
echo ""

# Copy server script
cp coap_server.py /tmp/coap_server.py

# Use Python aiocoap server
docker run -d \
    --name coap-server \
    -p 5683:5683/udp \
    -v /tmp/coap_server.py:/app/server.py \
    python:3.11-alpine \
    sh -c "pip install aiocoap && python /app/server.py"

if [ $? -eq 0 ]; then
    echo ""
    echo "[✓] CoAP Server started successfully!"
    echo ""
    echo "Server details:"
    echo "  - Container name: coap-server"
    echo "  - CoAP port: 5683/udp"
    echo "  - Protocol: aiocoap server"
    echo "  - IP address: $(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' coap-server)"
    echo ""
    echo "To test connection (from inside container):"
    echo "  docker exec coap-server python -m aiocoap.cli.client GET coap://localhost/.well-known/core"
    echo ""
    echo "To stop server:"
    echo "  docker stop coap-server"
    echo ""
else
    echo ""
    echo "[✗] Failed to start CoAP server!"
    exit 1
fi
