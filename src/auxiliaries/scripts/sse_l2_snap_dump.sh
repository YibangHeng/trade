#!/usr/bin/env bash

mkdir -p /var/raw_md_recorder/"$(date +%Y%m%d)"

tcpdump -i enp152s0f0np0 -B 131072 \
        -w /var/raw_md_recorder/"$(date +%Y%m%d)"/sse_l2_snap.pcap \
        "udp and dst host 233.57.1.112 and dst port 37112"
