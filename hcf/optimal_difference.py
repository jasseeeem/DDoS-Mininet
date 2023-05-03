from scapy.all import *

def send_tcp_packets(dest_ip, hop_limit):
    # Define the packet
    packet = IPv6(dst=dest_ip, hlim=hop_limit) / TCP()

    # Send the packet in a loop
    while True:
        send(packet)

        # Wait for 1 second before sending the next packet
        time.sleep(1)

        # Record the hop limit in a file
        with open("hop_limits.txt", "a") as f:
            f.write(f"{hop_limit}\n")

if __name__ == "__main__":
    dest_ip = "2401:4900:32f7:4a8b:c73a:9f3f:a0b5:6f04"  # Replace with the destination IPv6 address
    hop_limit = 64  # Replace with the initial hop limit value

    # Start sending packets in a new thread
    thread = threading.Thread(target=send_tcp_packets, args=(dest_ip, hop_limit))
    thread.start()

    # Continuously monitor the hop limit value and update the packet
    while True:
        # Receive a packet and extract the hop limit value
        packet = sniff(filter=f"ipv6 dst {dest_ip} and tcp", count=1)[0]
        new_hop_limit = packet.hlim - 1

        # Update the packet with the new hop limit value
        packet.hlim = new_hop_limit

        # Update the hop limit variable
        hop_limit = new_hop_limit

        # Record the hop limit in a file
        with open("hop_limit.txt", "a") as f:
            f.write(f"{hop_limit}\n")
