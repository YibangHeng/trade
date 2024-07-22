#!/bin/bash

mkdir  -p /var/raw_md_recorder/"$(date +%Y%m%d)"

tshark -i enp152s0f0np0 \
       -f "udp and (dst host 233.57.1.126 and dst port 37126)" \
       -T fields -e data \
       -w /var/raw_md_recorder/"$(date +%Y%m%d)"/sse_tick.pcap
