# Zenoh-Pico Attack Examples

Security attack suite for Zenoh protocol - **FOR EDUCATIONAL PURPOSES ONLY**

---

## 📖 What is Zenoh?

**Zenoh** is a modern pub/sub/query protocol for IoT, robotics, and edge computing.

### Real-World Architecture:

```
┌─────────────┐         ┌─────────────┐         ┌─────────────┐
│   Device    │────────▶│   Zenoh     │◀────────│   Device    │
│ (Zenoh-Pico)│  7447   │   Router    │  7447   │ (Zenoh-Pico)│
└─────────────┘         └─────────────┘         └─────────────┘
  Sensor/MCU              zenohd (TCP/UDP)        Actuator/MCU
  Publisher                                       Subscriber
```

### Our Test Setup:

```
┌─────────────────┐                    ┌─────────────┐
│ Attack Programs │───Attack Traffic──▶│   Zenoh     │
│ (bin/attack*)   │       7447         │   Router    │
│                 │                    │  (zenohd)   │
│ Simulate        │   Malicious        │             │
│ Zenoh-Pico      │   Packets          │   TARGET    │
│ Clients         │                    │             │
└─────────────────┘                    └─────────────┘
```

**Components:**

- **Zenoh Router** (`zenohd`) - Central broker that routes messages between devices - **THIS IS THE TARGET**
- **Zenoh-Pico** - Lightweight C implementation for microcontrollers/embedded devices
- **Our Attack Programs** - Simulate Zenoh-Pico clients but send malicious traffic instead of legitimate data

**Key Point:**

- We **DON'T** need to install Zenoh-Pico library
- Our attack programs manually craft Zenoh protocol packets
- They act like malicious Zenoh-Pico devices to attack the router
- The router (zenohd) is the target we're testing for vulnerabilities

**What these attacks do:**
Send malformed, excessive, or malicious packets to the Zenoh router to exploit protocol vulnerabilities (DoS, protocol flaws, resource exhaustion).

---

## 🚀 Quick Start

```bash
# 1. Compile attacks
chmod +x compile_all.sh run_tests.sh
./compile_all.sh

# 2. Start Zenoh router
docker run -d --name zenoh-router -p 7447:7447 eclipse/zenoh:1.7.2

# 3. Run interactive test menu
./run_tests.sh

# Or run the attacks indivually

./bin/attack9_memexhaust 127.0.0.1 7447 5   # Memory exhaustion

# 4. Clean up
docker stop zenoh-router && docker rm zenoh-router
```

---

## 📋 Requirements

- Linux or WSL
- GCC compiler (`sudo apt install build-essential`)
- Docker

---

## 📊 Available Attacks (19 total)

| #   | Attack Name             | Type     | Command Example                               |
| --- | ----------------------- | -------- | --------------------------------------------- |
| 1   | KeepAlive Flood         | DoS      | `./bin/attack1_keepalive 127.0.0.1 7447 10`   |
| 2   | Malformed Init          | DoS      | `./bin/attack2_malformed 127.0.0.1 7447 5`    |
| 3   | Fragment Bomb           | DoS      | `./bin/attack3_fragbomb 127.0.0.1 7447 5`     |
| 4   | Session Hijack          | Protocol | `sudo ./bin/attack4_hijack 127.0.0.1 7447`    |
| 5   | Protocol Fuzzer         | Protocol | `./bin/attack5_fuzzer 127.0.0.1 7447 1000`    |
| 6   | Replay Attack           | Protocol | `sudo ./bin/attack6_replay 127.0.0.1 7447`    |
| 7   | Amplification           | Protocol | `./bin/attack7_amplify 127.0.0.1 7447 <IP> 5` |
| 8   | Sequence Exhaustion     | Resource | `./bin/attack8_seqexhaust 127.0.0.1 7447 5`   |
| 9   | Memory Exhaustion       | Resource | `./bin/attack9_memexhaust 127.0.0.1 7447 3`   |
| 10  | Slowloris               | DoS      | `./bin/attack10_slowloris 127.0.0.1 7447 500` |
| 11  | Router Spoof            | Protocol | `sudo ./bin/attack11_routerspoof <IP> <IP> 5` |
| 12  | Fragment Confusion      | Protocol | `./bin/attack12_fragconfuse 127.0.0.1 7447 5` |
| 13  | Timestamp Manipulation  | Protocol | `./bin/attack13_timestamp 127.0.0.1 7447 5`   |
| 14  | QoS Inversion           | Resource | `./bin/attack14_qos 127.0.0.1 7447 5`         |
| 15  | Subscription Exhaustion | Resource | `./bin/attack15_subexhaust 127.0.0.1 7447 5`  |
| 16  | Wildcard Query Bomb     | DoS      | `./bin/attack16_querybomb 127.0.0.1 7447 5`   |
| 17  | Liveliness Flood        | DoS      | `./bin/attack17_liveliness 127.0.0.1 7447 5`  |
| 19  | Attachment Bomb         | DoS      | `./bin/attack19_attachment 127.0.0.1 7447 5`  |
| 20  | KeyExpr Collision       | Protocol | `./bin/attack20_keyexpr 127.0.0.1 7447 5`     |

> **Note:** Attacks 4, 6, 11 require `sudo` for raw socket access

---

## ✅ How to Verify Attacks Work

**Quick validation:**

```bash
# Terminal 1: Monitor router
docker logs -f zenoh-router

# Terminal 2: Run attack
./bin/attack1_keepalive 127.0.0.1 7447 5
```

**Signs attack is working:**

- ✅ Router logs show "Accepted TCP connection" messages
- ✅ CPU usage spikes (`docker stats zenoh-router`)
- ✅ Many "Read error" messages in logs
- ✅ Router becomes slow/unresponsive (successful DoS!)

**Failed attack indicators:**

- ❌ No log activity in router
- ❌ CPU stays at 0-1%
- ❌ No network traffic
- ❌ Zero connections

---

## 🔧 Troubleshooting

**Connection refused:**

```bash
docker ps                     # Check router is running
docker logs zenoh-router      # Check for errors
```

**Compilation errors:**

```bash
sudo apt install build-essential gcc make
./compile_all.sh
```

**No Docker:**

```bash
# Use native installation (Ubuntu 20.04+)
./setup_zenoh.sh              # Install zenohd
zenohd                        # Run router
```

---

## 📚 Resources

- **Zenoh Website:** https://zenoh.io/
- **Zenoh-Pico GitHub:** https://github.com/eclipse-zenoh/zenoh-pico
- **Zenoh Router GitHub:** https://github.com/eclipse-zenoh/zenoh

---

## ⚠️ Legal Warning

**FOR EDUCATIONAL USE ONLY**

- Only test on systems you own
- Never use on production systems
- Unauthorized testing is illegal
- Use isolated environments

---

## 📝 License

MIT License - See LICENSE file
