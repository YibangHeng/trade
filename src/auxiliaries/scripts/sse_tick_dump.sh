#!/usr/bin/env bash

mkdir  -p /var/raw_md_recorder/"$(date +%Y%m%d)"

tshark -i enp152s0f0np0 -B 131072 \
       -w /var/raw_md_recorder/"$(date +%Y%m%d)"/sse_tick.pcap \
       "udp and dst host 233.57.1.126 and dst port 37126"
