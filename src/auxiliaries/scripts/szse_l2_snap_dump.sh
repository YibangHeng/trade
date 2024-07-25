#!/usr/bin/env bash

mkdir  -p /var/raw_md_recorder/"$(date +%Y%m%d)"

tshark -i enp152s0f0np0 -B 131072 \
       -w /var/raw_md_recorder/"$(date +%Y%m%d)"/szse_l2_snap.pcap \
       "udp and dst host 233.58.2.100 and dst port 38200"
