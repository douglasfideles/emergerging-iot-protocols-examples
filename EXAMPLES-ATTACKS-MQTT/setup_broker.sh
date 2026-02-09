#!/bin/bash

echo "======================================================================"
echo "  Setting up MQTT Broker (Mosquitto) - GT-IoTEdu"
echo "======================================================================"

# Check if mosquitto container already exists
if [ "$(docker ps -aq -f name=mqtt-broker)" ]; then
    echo "[*] Stopping existing mqtt-broker container..."
    docker stop mqtt-broker 2>/dev/null
    docker rm mqtt-broker 2>/dev/null
fi

echo ""
echo "[*] Starting Mosquitto MQTT broker..."
echo "[*] Port: 1883 (MQTT)"
echo ""

# Create a temporary mosquitto config
cat > /tmp/mosquitto-no-auth.conf <<EOF
listener 1883
allow_anonymous true
persistence false
EOF

docker run -d \
    --name mqtt-broker \
    -p 1883:1883 \
    -v /tmp/mosquitto-no-auth.conf:/mosquitto/config/mosquitto.conf \
    eclipse-mosquitto:latest

if [ $? -eq 0 ]; then
    echo ""
    echo "[✓] MQTT Broker started successfully!"
    echo ""
    echo "Broker details:"
    echo "  - Container name: mqtt-broker"
    echo "  - MQTT port: 1883"
    echo "  - IP address: $(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' mqtt-broker)"
    echo ""
    echo "To test connection:"
    echo "  docker exec mqtt-broker mosquitto_sub -h localhost -t '#' -v"
    echo ""
    echo "To stop broker:"
    echo "  docker stop mqtt-broker"
    echo ""
else
    echo ""
    echo "[✗] Failed to start MQTT broker!"
    exit 1
fi
