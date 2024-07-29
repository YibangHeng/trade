#!/usr/bin/env bash

mkdir  -p /var/raw_md_recorder/"$(date +%Y%m%d)"

tshark -n -N n \
       -i enp152s0f0np0 -B 128 \
       -f "udp and ((dst host 233.57.1.112 and dst port 37112) or (dst host 233.58.2.100 and dst port 38200))" \
       -T fields -e data \
       -w /var/raw_md_recorder/"$(date +%Y%m%d)"/l2_snap.pcap
