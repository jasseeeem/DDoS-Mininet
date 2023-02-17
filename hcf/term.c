#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <netinet/ip6.h>

void print_ipv6_packet(const u_char *packet, int packet_size)
{
    // Get a pointer to the IPv6 header
    const struct ip6_hdr *ipv6_hdr = (const struct ip6_hdr *)(packet);

    // Print the source and destination addresses
    char src_addr[INET6_ADDRSTRLEN];
    char dst_addr[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &(ipv6_hdr->ip6_src), src_addr, INET6_ADDRSTRLEN);
    inet_ntop(AF_INET6, &(ipv6_hdr->ip6_dst), dst_addr, INET6_ADDRSTRLEN);
    printf("Source address: %s\n", src_addr);
    printf("Destination address: %s\n", dst_addr);
}

int main()
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *pcap_handle;
    const u_char *packet;
    struct pcap_pkthdr packet_header;

    // Open the default network interface for capturing packets
    pcap_handle = pcap_open_live("enp0s3", BUFSIZ, 1, 1000, errbuf);
    if (pcap_handle == NULL)
    {
        printf("Error opening network interface: %s\n", errbuf);
        exit(1);
    }
    printf("here");

    // Enter the capture loop
    while (1)
    {
        // Get the next packet from the network interface
        packet = pcap_next(pcap_handle, &packet_header);
        printf("here2");
        if (packet != NULL)
        {
            // Check if the packet is an IPv6 packet
            if (packet[0] >> 4 == 6)
            {
                // Print the IPv6 packet
                printf("h34");
                print_ipv6_packet(packet, packet_header.len);
                printf("hdf");
            }
        }
    }

    // pcap_close(pcap_handle);
    return 0;
}
