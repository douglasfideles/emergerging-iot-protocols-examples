#!/bin/bash

echo "╔════════════════════════════════════════════╗"
echo "║   Zenoh Attack Validation Monitor         ║"
echo "╚════════════════════════════════════════════╝"
echo ""

# Check if zenoh container is running
if ! docker ps | grep -q zenoh; then
    echo "[!] Zenoh router not running!"
    echo "[*] Start it with: docker run -d --name zenoh-router -p 7447:7447 eclipse/zenoh:1.7.2"
    exit 1
fi

echo "[✓] Zenoh router is running"
echo ""

# Get baseline stats
echo "[*] Collecting baseline stats (5 seconds)..."
BASELINE_RX=$(docker stats zenoh-router --no-stream --format "{{.NetIO}}" | cut -d'/' -f1)
sleep 5

echo ""
echo "╔════════════════════════════════════════════╗"
echo "║   Validation Methods                       ║"
echo "╚════════════════════════════════════════════╝"
echo ""
echo "1. Network Connections:"
netstat -an 2>/dev/null | grep 7447 | head -10 || ss -tan | grep 7447 | head -10
echo ""

echo "2. Docker Container Stats:"
docker stats zenoh-router --no-stream
echo ""

echo "3. Recent Router Logs (last 20 lines):"
docker logs zenoh-router --tail 20
echo ""

echo "4. Port 7447 Traffic (capture 10 packets):"
echo "   Note: Requires root/sudo for tcpdump"
timeout 3 sudo tcpdump -i any port 7447 -c 10 -n 2>/dev/null || echo "   (tcpdump not available or no sudo access)"
echo ""

echo "╔════════════════════════════════════════════╗"
echo "║   Real-time Monitoring Commands            ║"
echo "╚════════════════════════════════════════════╝"
echo ""
echo "Monitor logs:          docker logs -f zenoh-router"
echo "Monitor stats:         docker stats zenoh-router"
echo "Monitor connections:   watch -n 1 'netstat -an | grep 7447 | wc -l'"
echo "Monitor traffic:       sudo tcpdump -i any port 7447 -n"
echo ""
