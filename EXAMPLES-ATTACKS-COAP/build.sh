#!/bin/bash

echo "======================================================================"
echo "  Building CoAP Attacks Docker Image - GT-IoTEdu"
echo "======================================================================"

IMAGE_NAME="iotedu-coap-attacks"

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
    echo "  docker run --rm --network host ${IMAGE_NAME}:latest attack-coap-resource-exhaustion_attack.py localhost 5683"
    echo ""
    echo "Available attacks:"
    echo "  - attack-coap-block-fragmentation_attack.py"
    echo "  - attack-coap-observe-amplification_attack.py"
    echo "  - attack-coap-token-collision_attack.py"
    echo "  - attack-coap-multicast-amplification_attack.py"
    echo "  - attack-coap-resource-exhaustion_attack.py"
    echo "  - attack-coap-message-id-replay_attack.py"
    echo "  - attack-coap-proxy-amplification_attack.py"
    echo "  - attack-coap-response-fuzzing_attack.py"
    echo ""
else
    echo ""
    echo "[✗] Build failed!"
    exit 1
fi
