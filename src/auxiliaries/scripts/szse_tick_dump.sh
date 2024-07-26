#!/usr/bin/env bash

mkdir -p /var/raw_md_recorder/"$(date +%Y%m%d)"

tcpdump -i enp152s0f0np0 -B 131072 \
        -w /var/raw_md_recorder/"$(date +%Y%m%d)"/szse_tick.pcap \
        "udp and dst host 233.58.2.102 and dst port 38202"
