#!/usr/bin/env python

from scapy.all import *

# Define the source and destination addresses
src_addr = "2600:1901:0000:e988:0000:0000:0000:0000"
dst_addr = "2401:4900:32f7:4a8b:c73a:9f3f:a0b5:6f04"

# Define the IPv6 packet with the custom source address
ipv6_pkt = IPv6(src=src_addr, dst=dst_addr)

# Define the TCP packet with custom source and destination ports
tcp_pkt = TCP(sport=1234, dport=80)

# Combine the packets into a single packet
pkt = ipv6_pkt / tcp_pkt

# Send the packet on the "eth0" interface
send(pkt, iface="enp0s3")