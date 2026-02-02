#!/bin/bash
# Compile all 13 Zenoh-Pico attacks

echo "╔═════════════════════════════���══════════════╗"
echo "║  Compiling All Zenoh-Pico Attacks          ║"
echo "║  WARNING: EDUCATIONAL PURPOSES ONLY        ║"
echo "╚════════════════════════════════════════════╝"
echo ""

attacks=(
    "attack_1_dos_keepalive:attack1_keepalive"
    "attack_2_dos_malformed_init:attack2_malformed"
    "attack_3_fragment_bomb:attack3_fragbomb"
    "attack_4_session_hijack:attack4_hijack"
    "attack_5_protocol_fuzzer:attack5_fuzzer"
    "attack_6_replay:attack6_replay"
    "attack_7_amplification:attack7_amplify"
    "attack_8_sequence_exhaustion:attack8_seqexhaust"
    "attack_9_memory_exhaustion:attack9_memexhaust"
    "attack_10_slowloris:attack10_slowloris"
    "attack_11_router_spoof:attack11_routerspoof"
    "attack_12_fragment_confusion:attack12_fragconfuse"
    "attack_13_timestamp_manipulation:attack13_timestamp"
)

compiled=0
failed=0

for attack in "${attacks[@]}"; do
    IFS=':' read -r source binary <<< "$attack"
    
    printf "Compiling %-35s ... " "$source.c"
    
    if gcc -o "$binary" "$source.c" -pthread -O2 2>/dev/null; then
        echo "✓ OK"
        ((compiled++))
    else
        echo "✗ FAILED"
        ((failed++))
    fi
done

echo ""
echo "════════════════════════════════════════════"
echo "Compilation complete!"
echo "  Success: $compiled"
echo "  Failed:  $failed"
echo "════════════════════════════════════════════"
echo ""

if [ $compiled -gt 0 ]; then
    echo "✓ Binaries created in: ./bin/"
    echo ""
    echo "Usage examples:"
    echo "  ./bin/attack1_keepalive 127.0.0.1 7447 10"
    echo "  ./bin/attack2_malformed 127.0.0.1 7447 5"
    echo "  ./bin/attack3_fragbomb 127.0.0.1 7447 5"
    echo "  sudo ./bin/attack4_hijack 127.0.0.1 7447"
    echo "  ./bin/attack5_fuzzer 127.0.0.1 7447 1000"
    echo "  sudo ./bin/attack6_replay 127.0.0.1 7447"
    echo "  ./bin/attack7_amplify 127.0.0.1 7447 192.168.1.100 5"
    echo "  ./bin/attack8_seqexhaust 127.0.0.1 7447 5"
    echo "  ./bin/attack9_memexhaust 127.0.0.1 7447 3"
    echo "  ./bin/attack10_slowloris 127.0.0.1 7447 500"
    echo "  ./bin/attack11_routerspoof 192.168.1.255 192.168.1.100 5"
    echo "  ./bin/attack12_fragconfuse 127.0.0.1 7447 5"
    echo "  ./bin/attack13_timestamp 127.0.0.1 7447 5"
    echo "  ./bin/attack14_qos 127.0.0.1 7447 5"
    echo "  ./bin/attack15_subexhaust 127.0.0.1 7447 5"
    echo "  ./bin/attack16_querybomb 127.0.0.1 7447 5"
    echo "  ./bin/attack17_liveliness 127.0.0.1 7447 5"
    echo "  ./bin/attack19_attachment 127.0.0.1 7447 5"
    echo "  ./bin/attack20_keyexpr 127.0.0.1 7447 5"
fi

if [ $failed -gt 0 ]; then
    echo ""
    echo "⚠ Some attacks failed to compile."
    echo "  Check error messages above."
    exit 1
fi
    echo "  ./bin/attack7_amplify 127.0.0.1 7447 192.168.1.100 5"
    echo "  ./bin/attack8_seqexhaust 127.0.0.1 7447 5"
    echo "  ./bin/attack9_memexhaust 127.0.0.1 7447 3"
    echo "  ./bin/attack10_slowloris 127.0.0.1 7447 500"
    echo "  ./bin/attack11_routerspoof 192.168.1.255 192.168.1.100 5"
    echo "  ./bin/attack12_fragconfuse 127.0.0.1 7447 5"
    echo "  ./bin/attack13_timestamp 127.0.0.1 7447 5"
    echo "  ./bin/attack14_qos 127.0.0.1 7447 5"
    echo "  ./bin/attack15_subexhaust 127.0.0.1 7447 5"
    echo "  ./bin/attack16_querybomb 127.0.0.1 7447 5"
    echo "  ./bin/attack17_liveliness 127.0.0.1 7447 5"
    echo "  ./bin/attack19_attachment 127.0.0.1 7447 5"
    echo "  ./bin/attack20_keyexpr 127.0.0.1 7447 5"
fi

if [ $failed -gt 0 ]; then
    echo ""
    echo "⚠ Some attacks failed to compile."
    echo "  Check error messages above."
    exit 1
fi