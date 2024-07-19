#!/bin/bash

tshark -i enp152s0f0np0 \
       -f "udp and (dst host 233.57.1.126 and dst port 37126)" \
       -T fields -e data \
       | xxd -r -p > /var/raw_md_recorder/"$(date +%Y%m%d)"/sse_tick.rt
