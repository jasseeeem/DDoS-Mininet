#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <netinet/ip6.h>
#include <net/ethernet.h>

void packet_handler(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);

int main(int argc, char *argv[])
{
    char *dev;                     /* The device to sniff on */
    char errbuf[PCAP_ERRBUF_SIZE]; /* Error string */
    pcap_t *handle;                /* Session handle */
    struct bpf_program fp;         /* The compiled filter expression */
    char filter_exp[] = "ip";      /* The filter expression */
    bpf_u_int32 mask;              /* Our netmask */
    bpf_u_int32 net;               /* Our IP */
    int num_packets = -1;          /* Number of packets to capture */

    /* Define the device */
    dev = "enp0s3";

    /* Find the properties for the device */
    if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1)
    {
        fprintf(stderr, "Couldn't get netmask for device %s: %s\n", dev, errbuf);
        net = 0;
        mask = 0;
    }

    /* Open the session in promiscuous mode */
    handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL)
    {
        fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
        return 2;
    }

    /* Compile and apply the filter expression */
    if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1)
    {
        fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
        return 2;
    }
    if (pcap_setfilter(handle, &fp) == -1)
    {
        fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
        return 2;
    }

    /* Start capturing packets */
    pcap_loop(handle, num_packets, packet_handler, NULL);

    /* Close the session */
    pcap_close(handle);

    return 0;
}

void packet_handler(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
    struct ether_header *eth_header;
    struct ip6_hdr *ipv6_header;
    u_int16_t ethertype;
    int size;

    eth_header = (struct ether_header *)packet;
    ethertype = ntohs(eth_header->ether_type);

    if (ethertype == ETHERTYPE_IPV6)
    { // IPv6 packet

        ipv6_header = (struct ip6_hdr *)(packet + sizeof(struct ether_header));
        size = header->len;

        if (ipv6_header->ip6_nxt == IPPROTO_TCP)
        { // TCP packet

            // Extract the hop limit
            int hop_limit = ipv6_header->ip6_hlim;

            // Print the hop limit to the console
            printf("Hop Limit: %d\n", hop_limit);
        }
    }
}