# MQTT Protocol Attack Examples

Examples of security vulnerabilities and attack vectors in MQTT protocol implementations for IoT education purposes.

## Overview

This folder contains 6 different MQTT attack demonstrations:

1. **LWT Abuse** - Exploits Last Will Testament feature (50 fake critical devices)
2. **Topic Injection** - Attempts unauthorized access to admin/system topics
3. **QoS Amplification** - Amplifies traffic using QoS 2 handshakes (16,000 packets)
4. **Session Hijack** - Hijacks client sessions using duplicate IDs
5. **Retained Poison** - Poisons retained messages with false critical data
6. **Wildcard Enumeration** - Enumerates all topics using wildcards

## Prerequisites

- Docker installed (with WSL2 for Windows users)
- Python 3.x (optional, for non-Docker execution)

## Quick Start

### For Linux/WSL Direct Users

# 1. Setup broker

chmod +x setup_broker.sh
./setup_broker.sh

# 2. Build attack image (already built, but if needed)

chmod +x build.sh
./build.sh

# 3. Run all attacks

chmod +x test_all.sh
./test_all.sh

````

## Running Individual Attacks

### Using Docker (Recommended)

```bash
# LWT Abuse - Creates 50 fake device failures
docker run --rm --network host iotedu-mqtt-attacks:latest attack-mqtt-lwt-abuse.py localhost

# Topic Injection - Attempts to access restricted topics
timeout 10 docker run --rm --network host iotedu-mqtt-attacks:latest attack-mqtt-topic-injection.py localhost

# QoS Amplification - Generates massive traffic
docker run --rm --network host iotedu-mqtt-attacks:latest attack-mqtt-qos-amplification.py localhost

# Session Hijack - Takes over client connections
docker run --rm --network host iotedu-mqtt-attacks:latest attack-mqtt-session-hijack.py localhost

# Retained Poison - Injects false critical messages
timeout 5 docker run --rm --network host iotedu-mqtt-attacks:latest attack-mqtt-retained-poison.py localhost

# Wildcard Enumeration - Discovers all topics
timeout 15 docker run --rm --network host iotedu-mqtt-attacks:latest attack-mqtt-wildcard-enumeration.py localhost
````

### Using Python Directly (Without Docker)

```bash
# Install dependencies
pip install -r requirements.txt

# Run any attack
python attack-mqtt-lwt-abuse.py localhost
python attack-mqtt-topic-injection.py localhost
python attack-mqtt-qos-amplification.py localhost
```

## Attack Details

| Attack                   | Script                                | Impact                                     | What It Does                                                                                                                                 |
| ------------------------ | ------------------------------------- | ------------------------------------------ | -------------------------------------------------------------------------------------------------------------------------------------------- |
| **LWT Abuse**            | `attack-mqtt-lwt-abuse.py`            | Resource exhaustion, false alerts          | Creates 50 fake critical devices that trigger Last Will Testament messages when disconnecting, flooding the system with false failure alerts |
| **Topic Injection**      | `attack-mqtt-topic-injection.py`      | Privilege escalation, unauthorized control | Attempts to publish to administrative topics like `$SYS/broker/config`, `admin/commands`, and industrial control topics                      |
| **QoS Amplification**    | `attack-mqtt-qos-amplification.py`    | DoS, bandwidth exhaustion                  | Uses 20 clients × 200 QoS 2 messages = 16,000 MQTT packets (4-way handshake per message)                                                     |
| **Session Hijack**       | `attack-mqtt-session-hijack.py`       | Connection disruption, impersonation       | Connects with common device Client IDs to forcibly disconnect legitimate devices                                                             |
| **Retained Poison**      | `attack-mqtt-retained-poison.py`      | Data integrity, persistent false data      | Publishes false critical messages with `retain=True` that persist on broker and mislead new subscribers                                      |
| **Wildcard Enumeration** | `attack-mqtt-wildcard-enumeration.py` | Information disclosure                     | Subscribes to `#` and `$SYS/#` wildcards to discover all topics and system information                                                       |

## Monitoring and Debugging

### Monitor All MQTT Traffic

In a separate terminal, monitor all broker traffic:

```bash
# From WSL/Linux
docker exec mqtt-broker mosquitto_sub -h localhost -t '#' -v

# From Windows PowerShell
wsl docker exec mqtt-broker mosquitto_sub -h localhost -t '#' -v
```

### Check Broker Status

```bash
# List running containers
docker ps | grep mqtt-broker

# View broker logs
docker logs mqtt-broker

# Get broker IP address
docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' mqtt-broker
```

### Publish Test Messages

````bash
# Publish a test message
docker exec mqtt-broker mosquitto_pub -h localhost -t test/topic -m "Hello MQTT"

# Subscribe to specific topic
docker exec mqtt-broker mosquitto_sub -h localhost -t test/topic -v
```Project Structure

````

EXAMPLES-ATTACKS-MQTT/
├── Dockerfile # Single Docker image for all attacks
├── requirements.txt # Python dependencies (paho-mqtt)
├── mosquitto.conf # Broker configuration (no auth)
├── setup_broker.sh # Automated broker deployment
├── build.sh # Build Docker image
├── run_attack.sh # Run individual attacks
├── test_all.sh # Test all attacks (Bash)
├── test_all.ps1 # Test all attacks (PowerShell)
├── attack-mqtt-lwt-abuse.py # Attack scripts
├── attack-mqtt-topic-injection.py
├── attack-mqtt-qos-amplification.py
├── attack-mqtt-session-hijack.py
├── attack-mqtt-retained-poison.py
└── attack-mqtt-wildcard-enumeration.py

````

## Technical Architecture

- **Single Dockerfile**: One image contains all 6 attack scripts (efficient, no duplication)
- **Base Image**: Python 3.11 Alpine (~60MB total image size)
- **Dependencies**: Only `paho-mqtt` MQTT client library
- **Network**: Uses `--network host` for direct localhost access
- **Broker**: Eclipse Mosquitto with no authentication (for testing)

## Security Mitigations to Learn

After running attacks on unprotected broker, implement these mitigations:

### 1. Access Control Lists (ACLs)
```conf
# mosquitto.conf
acl_file /mosquitto/config/acl.conf
````

### 2. Authentication

```conf
# mosquitto.conf
allow_anonymous false
password_file /mosquitto/config/passwd
```

### 3. TLS Encryption

```conf
# mosquitto.conf
listener 8883
cafile /mosquitto/config/ca.crt
certfile /mosquitto/config/broker.crt
keyfile /mosquitto/config/broker.key
```

### 4. Rate Limiting

Configure `max_connections`, `max_inflight_messages`, and `max_queued_messages`

### 5. Topic Restrictions

```conf
# acl.conf
user sensor1
topic read sensors/#
topic write sensors/sensor1/#
```

## Learning Path

1. **Run attacks** - See them work against unprotected broker
2. **Add ACLs** - Configure Mosquitto with access control
3. **Enable authentication** - Add username/password
4. **Add TLS** - Enable encrypted communication
5. **Re-run attacks** - Observe how mitigations block attacks
6. **Compare results** - Document effectiveness of each mitigation

## Security Notes

⚠️ **Educational Purpose Only** - These attacks are for security research and education. Use only on systems you own or have explicit permission to test.

## References

- [MQTT v3.1.1 Specification](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html)
- [OWASP IoT Top 10](https://owasp.org/www-project-internet-of-things/)
- [Mosquitto MQTT Broker](https://mosquitto.org/)
- [Mosquitto Security](https://mosquitto.org/documentation/authentication-methods
- **Port 1883 in use**: Check with `netstat -an | findstr 1883` (Windows) or `sudo lsof -i :1883` (Linux)
- **Stop existing broker**: `docker stop mqtt-broker && docker rm mqtt-broker`

### Attacks Don't Connect

- **Verify broker is running**: `docker ps | grep mqtt-broker`
- **Check broker IP**: Use `docker inspect mqtt-broker | grep IPAddress`
- **Use correct IP**: Replace `localhost` with broker IP if needed

### Permission Errors (WSL)

- **Make scripts executable**: `chmod +x *.sh`
- **Run from correct path**: Use `/mnt/d/...` paths in WSL

## Security Notes

⚠️ **Educational Purpose Only** - These attacks are for security research and education. Use only on systems you own or have explicit permission to test.

## Mitigations

- Implement ACLs (Access Control Lists)
- Enable authentication (username/password or certificates)
- Use TLS encryption
- Implement rate limiting
- Monitor anomalous behavior
- Restrict `$SYS` topic access
- Validate client IDs

## References

- [MQTT v3.1.1 Specification](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html)
- [OWASP IoT Top 10](https://owasp.org/www-project-internet-of-things/)
- [Mosquitto MQTT Broker](https://mosquitto.org/)
