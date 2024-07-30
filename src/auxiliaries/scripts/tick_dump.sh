#!/usr/bin/env bash

mkdir  -p /var/raw_md_recorder/"$(date +%Y%m%d)"

tshark -n -N n \
       -i enp152s0f0np0 -B 128 \
       -f "udp and ((dst host 233.57.1.126 and dst port 37126) or (dst host 233.58.2.102 and dst port 38202))" \
       -T fields -e data \
       -w /var/raw_md_recorder/"$(date +%Y%m%d)"/tick.pcap
