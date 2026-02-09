#!/bin/bash

echo "======================================================================"
echo "  Testing MQTT Attacks - GT-IoTEdu"
echo "======================================================================"
echo ""

BROKER="localhost"

# Check if broker is running
if ! docker ps | grep -q mqtt-broker; then
    echo "[!] MQTT broker is not running. Starting it..."
    ./setup_broker.sh
    sleep 2
fi

echo "[*] Testing attacks against broker: $BROKER"
echo ""

# Test 1: LWT Abuse
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 1: Last Will Testament Abuse"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
docker run --rm --network host iotedu-mqtt-attacks:latest attack-mqtt-lwt-abuse.py $BROKER
echo ""
sleep 2

# Test 2: QoS Amplification
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 2: QoS Amplification Attack"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
docker run --rm --network host iotedu-mqtt-attacks:latest attack-mqtt-qos-amplification.py $BROKER
echo ""
sleep 2

# Test 3: Retained Poison
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 3: Retained Message Poisoning"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
timeout 5 docker run --rm --network host iotedu-mqtt-attacks:latest attack-mqtt-retained-poison.py $BROKER
echo ""
sleep 2

# Test 4: Session Hijack
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 4: Session Hijacking"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
docker run --rm --network host iotedu-mqtt-attacks:latest attack-mqtt-session-hijack.py $BROKER
echo ""
sleep 2

# Test 5: Topic Injection
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 5: Topic Injection"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
timeout 8 docker run --rm --network host iotedu-mqtt-attacks:latest attack-mqtt-topic-injection.py $BROKER
echo ""
sleep 2

# Test 6: Wildcard Enumeration
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 6: Wildcard Topic Enumeration"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
timeout 5 docker run --rm --network host iotedu-mqtt-attacks:latest attack-mqtt-wildcard-enumeration.py $BROKER
echo ""

echo "======================================================================"
echo "  All Tests Completed!"
echo "======================================================================"
echo ""
echo "To monitor broker in real-time:"
echo "  docker exec mqtt-broker mosquitto_sub -h localhost -t '#' -v"
echo ""
echo "To stop broker:"
echo "  docker stop mqtt-broker"
echo ""
