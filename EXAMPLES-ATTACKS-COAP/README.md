# CoAP Protocol Attack Examples

Examples of security vulnerabilities and attack vectors in CoAP (Constrained Application Protocol) implementations for IoT education purposes.

## Overview

This folder contains 8 different CoAP attack demonstrations targeting RFC 7252 (CoAP) and RFC 7959 (Block-Wise Transfer):

1. **Block Fragmentation** - Abuses block-wise transfer with incomplete/malformed fragments
2. **Observe Amplification** - Exploits Observe option for traffic amplification
3. **Token Collision** - Forces token collisions to confuse server state
4. **Multicast Amplification** - Uses multicast discovery for DRDoS attacks
5. **Resource Exhaustion** - Discovery and exhaustion of server resources
6. **Message ID Replay** - Replays message IDs to bypass deduplication
7. **Proxy Amplification** - Abuses CoAP proxy for amplification attacks
8. **Response Fuzzing** - Fuzzes responses to test server robustness

## Prerequisites

- Docker installed (with WSL2 for Windows users)
- Python 3.x (optional, for non-Docker execution)

## Quick Start

### For Linux/WSL Direct Users

```bash
cd /mnt/d/PROJETOS/react/Git/emergerging-iot-protocols-examples/EXAMPLES-ATTACKS-COAP

# 1. Setup CoAP server
chmod +x setup_server.sh
./setup_server.sh

# 2. Build attack image
chmod +x build.sh
./build.sh

# 3. Run all attacks
chmod +x test_all.sh
./test_all.sh
```

## Running Individual Attacks

### Using Docker (Recommended)

Get the server IP first:

```bash
SERVER_IP=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' coap-server)
echo "Server IP: $SERVER_IP"
```

Then run attacks:

```bash
# Resource Exhaustion - Discovers and exhausts server resources
timeout 10 docker run --rm --network bridge iotedu-coap-attacks:latest attack-coap-resource-exhaustion_attack.py $SERVER_IP 5683

# Token Collision - Forces token collisions
timeout 10 docker run --rm --network bridge iotedu-coap-attacks:latest attack-coap-token-collision_attack.py $SERVER_IP 5683

# Block Fragmentation - Sends malformed block-wise transfers
timeout 10 docker run --rm --network bridge iotedu-coap-attacks:latest attack-coap-block-fragmentation_attack.py $SERVER_IP 5683

# Observe Amplification - Registers multiple observers
timeout 10 docker run --rm --network bridge iotedu-coap-attacks:latest attack-coap-observe-amplification_attack.py $SERVER_IP 5683

# Multicast Amplification - DRDoS via multicast
docker run --rm --network bridge iotedu-coap-attacks:latest attack-coap-multicast-amplification_attack.py 224.0.1.187 5683

# Message ID Replay - Replays message IDs
timeout 10 docker run --rm --network bridge iotedu-coap-attacks:latest attack-coap-message-id-replay_attack.py $SERVER_IP 5683

# Proxy Amplification - Abuses proxy functionality
timeout 10 docker run --rm --network bridge iotedu-coap-attacks:latest attack-coap-proxy-amplification_attack.py $SERVER_IP 5683

# Response Fuzzing - Fuzzes server responses
timeout 10 docker run --rm --network bridge iotedu-coap-attacks:latest attack-coap-response-fuzzing_attack.py $SERVER_IP 5683
```

**Note:** The attacks will show "Connection loss was not expected" warnings - this is normal and expected behavior from the aiocoap library. The attacks ARE working if you see the server logs showing incoming requests.

### Using Python Directly (Without Docker)

```bash
# Install dependencies
pip install -r requirements.txt

# Run any attack
python attack-coap-resource-exhaustion_attack.py localhost 5683
python attack-coap-token-collision_attack.py localhost 5683
python attack-coap-block-fragmentation_attack.py localhost 5683
```

## Attack Details

| Attack                      | Script                                          | Impact                               | What It Does                                                                                                                          |
| --------------------------- | ----------------------------------------------- | ------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------- |
| **Block Fragmentation**     | `attack-coap-block-fragmentation_attack.py`     | Memory exhaustion, DoS               | Abuses RFC 7959 block-wise transfer by sending incomplete, overlapping, or out-of-order blocks that exhaust server reassembly buffers |
| **Observe Amplification**   | `attack-coap-observe-amplification_attack.py`   | Bandwidth amplification, DoS         | Registers multiple Observe subscriptions (RFC 7641) causing server to send continuous notifications, amplifying traffic               |
| **Token Collision**         | `attack-coap-token-collision_attack.py`         | State confusion, incorrect responses | Forces multiple requests to use same token value, confusing server's request/response matching logic                                  |
| **Multicast Amplification** | `attack-coap-multicast-amplification_attack.py` | DRDoS, bandwidth exhaustion          | Sends one multicast discovery request to `224.0.1.187`, triggering responses from all CoAP devices (1:N amplification)                |
| **Resource Exhaustion**     | `attack-coap-resource-exhaustion_attack.py`     | Resource discovery, DoS              | Queries `/.well-known/core` to discover all resources, then floods each resource with requests                                        |
| **Message ID Replay**       | `attack-coap-message-id-replay_attack.py`       | Bypass deduplication, replay attacks | Replays message IDs to test deduplication mechanisms and bypass message ID-based replay protection                                    |
| **Proxy Amplification**     | `attack-coap-proxy-amplification_attack.py`     | Amplification, unauthorized access   | Abuses CoAP proxy functionality to amplify traffic or access internal resources                                                       |
| **Response Fuzzing**        | `attack-coap-response-fuzzing_attack.py`        | Server crash, parser errors          | Sends malformed requests to trigger parsing errors and test server robustness                                                         |

## Monitoring and Debugging

### Monitor Server Logs

```bash
# From WSL/Linux
docker logs -f coap-server

# From Windows PowerShell
wsl docker logs -f coap-server
```

### Test Server Connectivity

```bash
# Using libcoap client (if available)
coap-client -m get coap://localhost/.well-known/core

# Using Python aiocoap
python -c "import asyncio; from aiocoap import *; asyncio.run((lambda: Context.create_client_context())())"
```

### Check Server Status

```bash
# List running containers
docker ps | grep coap-server

# Get server IP address
docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' coap-server
```

## Cleanup

```bash
# Stop and remove server
docker stop coap-server
docker rm coap-server

# Remove attack image
docker rmi iotedu-coap-attacks:latest

# Remove server image (if needed)
docker rmi python:3.11-alpine
```

## Troubleshooting

### Server Won't Start

- **Port 5683/udp in use**: Check with `netstat -an | findstr 5683` (Windows) or `sudo lsof -i :5683` (Linux)
- **Stop existing server**: `docker stop coap-server && docker rm coap-server`

### Attacks Don't Connect

- **Verify server is running**: `docker ps | grep coap-server`
- **Check server IP**: Use `docker inspect coap-server | grep IPAddress`
- **Use correct protocol**: CoAP uses UDP, not TCP

### Permission Errors (WSL)

- **Make scripts executable**: `chmod +x *.sh`
- **Run from correct path**: Use `/mnt/d/...` paths in WSL

### Asyncio Warnings

- Ignore warnings about unclosed event loops - these are expected with aiocoap

## Project Structure

```
EXAMPLES-ATTACKS-COAP/
├── Dockerfile                                      # Single Docker image for all attacks
├── requirements.txt                                # Python dependencies (aiocoap)
├── setup_server.sh                                 # Automated CoAP server deployment
├── build.sh                                        # Build Docker image
├── test_all.sh                                     # Test all attacks (Bash)
├── attack-coap-block-fragmentation_attack.py      # Attack scripts
├── attack-coap-observe-amplification_attack.py
├── attack-coap-token-collision_attack.py
├── attack-coap-multicast-amplification_attack.py
├── attack-coap-resource-exhaustion_attack.py
├── attack-coap-message-id-replay_attack.py
├── attack-coap-proxy-amplification_attack.py
└── attack-coap-response-fuzzing_attack.py
```

## Technical Architecture

- **Single Dockerfile**: One image contains all 8 attack scripts (efficient, no duplication)
- **Base Image**: Python 3.11 Alpine (~70MB total image size)
- **Dependencies**: Only `aiocoap` CoAP client library
- **Network**: Uses `--network host` for direct localhost/UDP access
- **Server**: aiocoap-based CoAP server (Python, RFC 7252 compliant)

## Security Mitigations to Learn

After running attacks on unprotected server, implement these mitigations:

### 1. Rate Limiting

Limit requests per client IP to prevent flooding attacks

### 2. Block-Wise Transfer Limits

```
- Set maximum block size
- Timeout incomplete transfers aggressively
- Limit concurrent block-wise transfers per client
```

### 3. Observe Management

```
- Limit observers per resource
- Limit observers per client
- Set observe timeout and cancellation
```

### 4. DTLS Encryption

```
Use CoAPS (CoAP over DTLS) instead of plain CoAP:
- coaps://server:5684 (DTLS port)
- Requires certificates for authentication
```

### 5. Resource Access Control

```
- Implement resource-level ACLs
- Validate client permissions before processing
- Restrict access to sensitive resources
```

### 6. Input Validation

```
- Validate all CoAP options
- Sanitize payloads
- Reject malformed messages early
```

## Learning Path

1. **Run attacks** - See them work against unprotected CoAP server
2. **Add rate limiting** - Limit requests per client
3. **Configure DTLS** - Enable encrypted communication (CoAPS)
4. **Implement ACLs** - Add resource-level access control
5. **Re-run attacks** - Observe how mitigations block attacks
6. **Compare results** - Document effectiveness of each mitigation

## CoAP Protocol Reference

### Key RFCs

- **RFC 7252**: The Constrained Application Protocol (CoAP)
- **RFC 7959**: Block-Wise Transfers in CoAP
- **RFC 7641**: Observing Resources in CoAP
- **RFC 6690**: Constrained RESTful Environments (CoRE) Link Format
- **RFC 7925**: TLS/DTLS Profiles for the Internet of Things

### CoAP Message Structure

```
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|Ver| T |  TKL  |      Code     |          Message ID           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   Token (if any, TKL bytes) ...
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   Options (if any) ...
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|1 1 1 1 1 1 1 1|    Payload (if any) ...
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

## Security Notes

⚠️ **Educational Purpose Only** - These attacks are for security research and education. Use only on systems you own or have explicit permission to test.

## References

- [RFC 7252 - CoAP Protocol](https://www.rfc-editor.org/rfc/rfc7252)
- [RFC 7959 - Block-Wise Transfer](https://www.rfc-editor.org/rfc/rfc7959)
- [RFC 7641 - Observing Resources](https://www.rfc-editor.org/rfc/rfc7641)
- [OWASP IoT Top 10](https://owasp.org/www-project-internet-of-things/)
- [Eclipse Californium](https://www.eclipse.org/californium/)
- [aiocoap Library](https://aiocoap.readthedocs.io/)
