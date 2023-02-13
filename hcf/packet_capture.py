from scapy.all import *

def analyze_tcp_packet(packet):
    if packet.haslayer(TCP):
        # Get the TCP layer
        tcp_layer = packet[TCP]

        # Extract the relevant information from the TCP layer
        src_port = tcp_layer.sport
        dst_port = tcp_layer.dport
        seq = tcp_layer.seq
        ack = tcp_layer.ack
        flags = tcp_layer.flags

        # Print the extracted information
        print(f"Source Port: {src_port}")
        print(f"Destination Port: {dst_port}")
        print(f"Sequence Number: {seq}")
        print(f"Acknowledgment Number: {ack}")
        print(f"Flags: {flags}")

# Sniff TCP packets
sniff(prn=analyze_tcp_packet, filter="tcp", store=0)
