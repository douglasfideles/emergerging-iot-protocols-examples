# 🔒 Emerging IoT Protocols - Security Testing Suite

Educational security testing framework for emerging IoT protocols: DDS-XRCE and Zenoh-Pico.

**⚠️ WARNING: FOR EDUCATIONAL PURPOSES ONLY**

## 📋 Overview

This repository contains comprehensive attack suites for testing the security of two emerging IoT protocols:

1. **DDS-XRCE** (Data Distribution Service for eXtremely Resource Constrained Environments)
2. **Zenoh-Pico** (Zenoh protocol for constrained devices)

## 🎯 Quick Navigation

### 📁 Repository Structure

```
emergerging-iot-protocols-examples/
├── EXAMPLES-ATTACKS-DDSXRCE/       # DDS-XRCE attacks (7 attacks)
├── EXAMPLES-ATTACKS-ZENOHPICO/     # Zenoh-Pico attacks (19 attacks)
├── COMPARISON_DDSXRCE_vs_ZENOHPICO.md  # Detailed comparison
└── README.md                        # This file
```

### 🚀 Quick Start

#### For DDS-XRCE Testing:

```bash
cd EXAMPLES-ATTACKS-DDSXRCE
./setup_dependencies.sh && ./compile_all.sh
./run_agent_docker.sh  # Terminal 1
./run_tests.sh  # Terminal 2
```

#### For Zenoh-Pico Testing:

```bash
cd EXAMPLES-ATTACKS-ZENOHPICO
./setup_zenoh.sh && ./compile_all.sh
zenohd  # Terminal 1
./run_tests.sh  # Terminal 2
```

## 📊 Attack Suites Comparison

| Feature            | DDS-XRCE                 | Zenoh-Pico        |
| ------------------ | ------------------------ | ----------------- |
| **Attacks**        | 7                        | 19                |
| **Setup Script**   | ✅ setup_dependencies.sh | ✅ setup_zenoh.sh |
| **Compile Script** | ✅ compile_all.sh        | ✅ compile_all.sh |
| **Test Script**    | ✅ run_tests.sh          | ✅ run_tests.sh   |
| **Setup Time**     | ~10 min                  | ~5 min            |
| **Docker Support** | ✅ Yes                   | ❌ No             |

## 🛠️ Environment Requirements

### Supported:

- **Linux** (Ubuntu, Debian, Fedora, Arch, etc.)
- **WSL Ubuntu** on Windows
- **GCC** 7.5+
- **Make**

### Tested On:

- **OS:** Windows 11 with WSL Ubuntu 20.04+
- **Compiler:** GCC 7.5+
- **Build Tools:** Make, CMake
- **Optional:** Docker (for DDS-XRCE agent)

### Prerequisites:

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y build-essential gcc make cmake git
```

## 📚 Documentation

### DDS-XRCE Suite

- [EXAMPLES-ATTACKS-DDSXRCE/README.md](EXAMPLES-ATTACKS-DDSXRCE/README.md) - Complete setup guide
- [DDSXRCE-EXAMPLE.md](DDSXRCE-EXAMPLE.md) - Protocol examples

### Zenoh-Pico Suite

- [EXAMPLES-ATTACKS-ZENOHPICO/README.md](EXAMPLES-ATTACKS-ZENOHPICO/README.md) - Protocol documentation
- [EXAMPLES-ATTACKS-ZENOHPICO/WSL_SETUP_GUIDE.md](EXAMPLES-ATTACKS-ZENOHPICO/WSL_SETUP_GUIDE.md) - WSL setup guide
- [EXAMPLES-ATTACKS-ZENOHPICO/QUICK_REFERENCE.md](EXAMPLES-ATTACKS-ZENOHPICO/QUICK_REFERENCE.md) - Command reference
- [EXAMPLES-ATTACKS-ZENOHPICO/VALIDATION_SUMMARY.md](EXAMPLES-ATTACKS-ZENOHPICO/VALIDATION_SUMMARY.md) - Validation status

### Comparison

- [COMPARISON_DDSXRCE_vs_ZENOHPICO.md](COMPARISON_DDSXRCE_vs_ZENOHPICO.md) - Detailed comparison

## 🎓 Attack Categories

### DDS-XRCE Attacks (7 Total)

1. **Session Hijack** - Hijack active sessions
2. **Entity Flood** - Overwhelm with entity requests
3. **Ping Flood** - Flood with ping messages
4. **Time Desync** - Timestamp manipulation
5. **Malformed Inject** - Inject invalid packets
6. **Fragment Abuse** - Exploit fragmentation
7. **Discovery Poison** - Poison discovery process

### Zenoh-Pico Attacks (19 Total)

**DoS Category (8 attacks):**

- KeepAlive Flood, Malformed Init, Fragment Bomb
- Sequence Exhaustion, Memory Exhaustion, Slowloris
- Subscription Exhaustion, Liveliness Flood

**Protocol Category (8 attacks):**

- Session Hijack, Protocol Fuzzer, Replay, Amplification
- Router Spoof, Fragment Confusion, Timestamp Manipulation
- KeyExpr Collision

**Resource Category (3 attacks):**

- QoS Inversion, Wildcard Query Bomb, Attachment Bomb

## 🔐 Security & Ethics

### ⚠️ CRITICAL WARNING

These tools are for **EDUCATIONAL PURPOSES ONLY**:

✅ **ALLOWED:**

- Educational learning
- Authorized security research
- Testing your own systems
- Isolated lab environments

❌ **FORBIDDEN:**

- Unauthorized testing
- Production systems
- Public networks
- Malicious use

**Unauthorized use is illegal and unethical.**

### Ethical Guidelines

1. Only test systems you own or have explicit permission to test
2. Use isolated lab environments
3. Document all testing activities
4. Never share attack results publicly
5. Report discovered vulnerabilities responsibly

## 🚀 Getting Started

### Step 1: Choose Your Protocol

**DDS-XRCE:**

- For testing DDS-XRCE implementations
- ROS 2 micro-ROS systems
- Embedded/microcontroller communications

**Zenoh-Pico:**

- For testing Zenoh implementations
- Edge computing systems
- Modern pub/sub protocols

### Step 2: Setup Environment

**Open WSL Ubuntu:**

```bash
# In Windows PowerShell
wsl
```

**Navigate to repository:**

```bash
cd /mnt/d/PROJETOS/react/Git/emergerging-iot-protocols-examples
```

### Step 3: Follow Suite-Specific Guide

**For DDS-XRCE:**

```bash
cd EXAMPLES-ATTACKS-DDSXRCE
cat README.md
```

**For Zenoh-Pico:**

```bash
cd EXAMPLES-ATTACKS-ZENOHPICO
cat WSL_SETUP_GUIDE.md
```

## 📖 Learning Path

### Beginner Path

1. Read protocol documentation
2. Set up DDS-XRCE suite (simpler, 7 attacks)
3. Run attacks against local agent
4. Understand attack mechanisms

### Advanced Path

1. Complete beginner path
2. Set up Zenoh-Pico suite (comprehensive, 19 attacks)
3. Compare attack effectiveness
4. Research defense mechanisms
5. Contribute improvements

## 🔧 Troubleshooting

### Common Issues

**"gcc: command not found"**

```bash
sudo apt update && sudo apt install build-essential
```

**"permission denied" on scripts**

```bash
chmod +x *.sh
```

**"cannot connect to target"**

```bash
# Make sure agent/router is running
# DDS-XRCE: ./run_agent_docker.sh
# Zenoh: zenohd
```

**WSL networking issues**

```bash
# Test localhost
ping 127.0.0.1
# Check WSL version
wsl --version
```

## 📊 Testing Workflow

### 1. Environment Validation

```bash
# DDS-XRCE
cd EXAMPLES-ATTACKS-DDSXRCE
./setup_dependencies.sh

# Zenoh-Pico
cd EXAMPLES-ATTACKS-ZENOHPICO
./validate_setup.sh
```

### 2. Compilation

```bash
# Both suites
./compile_all.sh
```

### 3. Target Setup

```bash
# DDS-XRCE (Terminal 1)
./run_agent_docker.sh

# Zenoh-Pico (Terminal 1)
zenohd
```

### 4. Run Tests

```bash
# DDS-XRCE (Terminal 2)
./run_tests.sh

# Zenoh-Pico (Terminal 2)
./quick_test.sh
```

## 📈 Project Status

### ✅ Completed

- [x] DDS-XRCE attack suite (7 attacks)
- [x] Zenoh-Pico attack suite (19 attacks)
- [x] WSL Ubuntu validation
- [x] Comprehensive documentation
- [x] Build systems (scripts + Makefiles)
- [x] Test frameworks
- [x] Comparison documentation

### 🎯 Validated

- [x] WSL Ubuntu 20.04+
- [x] GCC 7.5+
- [x] All attacks compile successfully
- [x] Test suites execute correctly

## 🤝 Contributing

This is an educational project. Contributions welcome:

- Bug fixes
- Documentation improvements
- New attack vectors
- Defense mechanisms
- Protocol updates

## 📄 License

Educational use only. See [LICENSE](LICENSE) file for details.

## 🔗 Resources

### DDS-XRCE

- [eProsima Micro XRCE-DDS](https://micro-xrce-dds.docs.eprosima.com/)
- [OMG DDS-XRCE Specification](https://www.omg.org/spec/DDS-XRCE/)
- [micro-ROS](https://micro.ros.org/)

### Zenoh

- [Zenoh Website](https://zenoh.io/)
- [Zenoh Documentation](https://zenoh.io/docs/)
- [Zenoh-Pico GitHub](https://github.com/eclipse-zenoh/zenoh-pico)
- [Eclipse Zenoh Project](https://projects.eclipse.org/projects/iot.zenoh)

### Security Research

- [OWASP IoT Top 10](https://owasp.org/www-project-internet-of-things/)
- [IoT Security Foundation](https://www.iotsecurityfoundation.org/)

## 📞 Support

For issues or questions:

1. Check documentation in respective suite folders
2. Review troubleshooting sections
3. Consult protocol documentation
4. Verify environment setup

## ⚡ Quick Command Reference

### DDS-XRCE

```bash
# Setup and compile
./setup_dependencies.sh && ./compile_all.sh

# Run agent
./run_agent_docker.sh

# Run attack
./bin/attack_session_hijack 127.0.0.1 2018
```

### Zenoh-Pico

```bash
# Validate and compile
./validate_setup.sh && ./compile_all.sh

# Run router
zenohd

# Run tests
./quick_test.sh

# Run specific attack
./run_attack.sh 5 127.0.0.1 7447 1000
```

## 🎯 Summary

This repository provides:

- ✅ **26 total attacks** (7 DDS-XRCE + 19 Zenoh-Pico)
- ✅ **WSL Ubuntu validated** setup
- ✅ **Comprehensive documentation**
- ✅ **Professional infrastructure**
- ✅ **Educational focus**

Perfect for learning IoT protocol security!

---

**Last Updated:** February 2026  
**Status:** Production Ready  
**Environment:** Windows WSL Ubuntu  
**Purpose:** Educational Security Research
