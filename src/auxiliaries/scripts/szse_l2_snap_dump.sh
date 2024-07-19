#!/bin/bash

tshark -i enp152s0f0np0 \
       -f "udp and (dst host 233.58.2.100 and dst port 38200)" \
       -T fields -e data \
       | xxd -r -p > /var/raw_md_recorder/"$(date +%Y%m%d)"/szse_l2_snap.rt
