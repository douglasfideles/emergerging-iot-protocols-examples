#!/bin/bash

# Script to run MQTT attacks using single Docker image
# Usage: ./run_attack.sh <attack-name> <broker-ip>

ATTACK_NAME=$1
BROKER_IP=${2:-"localhost"}

if [ -z "$ATTACK_NAME" ]; then
    echo "Usage: ./run_attack.sh <attack-name> <broker-ip>"
    echo ""
    echo "Available attacks:"
    echo "  1. lwt-abuse"
    echo "  2. topic-injection"
    echo "  3. qos-amplification"
    echo "  4. session-hijack"
    echo "  5. retained-poison"
    echo "  6. wildcard-enumeration"
    echo ""
    echo "Example: ./run_attack.sh topic-injection 172.17.0.2"
    exit 1
fi

# Map attack name to script file
case $ATTACK_NAME in
    "lwt-abuse"|"1")
        SCRIPT="attack-mqtt-lwt-abuse.py"
        ;;
    "topic-injection"|"2")
        SCRIPT="attack-mqtt-topic-injection.py"
        ;;
    "qos-amplification"|"3")
        SCRIPT="attack-mqtt-qos-amplification.py"
        ;;
    "session-hijack"|"4")
        SCRIPT="attack-mqtt-session-hijack.py"
        ;;
    "retained-poison"|"5")
        SCRIPT="attack-mqtt-retained-poison.py"
        ;;
    "wildcard-enumeration"|"6")
        SCRIPT="attack-mqtt-wildcard-enumeration.py"
        ;;
    *)
        echo "[!] Unknown attack: $ATTACK_NAME"
        exit 1
        ;;
esac

echo "[*] Running MQTT attack: $ATTACK_NAME"
echo "[*] Target broker: $BROKER_IP"
echo "[*] Script: $SCRIPT"
echo ""

docker run --rm \
    --network host \
    iotedu-mqtt-attacks:latest \
    $SCRIPT $BROKER_IP
