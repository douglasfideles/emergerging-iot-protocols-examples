#!/bin/bash

echo "======================================================================"
echo "  Building MQTT Attacks Docker Image - GT-IoTEdu"
echo "======================================================================"

IMAGE_NAME="iotedu-mqtt-attacks"

echo ""
echo "[*] Building: ${IMAGE_NAME}:latest"

docker build -t ${IMAGE_NAME}:latest .

if [ $? -eq 0 ]; then
    echo ""
    echo "[✓] Build successful!"
    echo ""
    echo "======================================================================"
    echo "  Usage Examples:"
    echo "======================================================================"
    echo ""
    echo "Run specific attack:"
    echo "  ./run_attack.sh topic-injection 172.17.0.2"
    echo "  ./run_attack.sh lwt-abuse localhost"
    echo ""
    echo "Run manually:"
    echo "  docker run --rm --network host ${IMAGE_NAME}:latest attack-mqtt-lwt-abuse.py 172.17.0.2"
    echo ""
    echo "Available attacks:"
    echo "  - attack-mqtt-lwt-abuse.py"
    echo "  - attack-mqtt-topic-injection.py"
    echo "  - attack-mqtt-qos-amplification.py"
    echo "  - attack-mqtt-session-hijack.py"
    echo "  - attack-mqtt-retained-poison.py"
    echo "  - attack-mqtt-wildcard-enumeration.py"
    echo ""
else
    echo ""
    echo "[✗] Build failed!"
    exit 1
fi
