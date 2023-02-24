#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <netinet/ip6.h>
#include <net/ethernet.h>

#define MAX_PACKETS 1

void process_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <IPv6 address>\n", argv[0]);
        return 1;
    }

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;
    struct bpf_program filter;
    char filter_exp[100];
    sprintf(filter_exp, "ip6 and src host %s", argv[1]);

    handle = pcap_open_live("enp0s3", BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL)
    {
        fprintf(stderr, "Couldn't open capture device: %s\n", errbuf);
        return 1;
    }

    if (pcap_compile(handle, &filter, filter_exp, 0, PCAP_NETMASK_UNKNOWN) == -1)
    {
        fprintf(stderr, "Couldn't compile filter: %s\n", pcap_geterr(handle));
        return 1;
    }

    if (pcap_setfilter(handle, &filter) == -1)
    {
        fprintf(stderr, "Couldn't set filter: %s\n", pcap_geterr(handle));
        return 1;
    }

    printf("Listening for traffic from %s...\n", argv[1]);

    pcap_loop(handle, MAX_PACKETS, process_packet, NULL);

    pcap_close(handle);
    return 0;
}

void process_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
    struct ip6_hdr *ipv6_header;
    u_int8_t *payload;
    u_int32_t payload_len;

    ipv6_header = (struct ip6_hdr *)(packet + sizeof(struct ether_header));
    payload = (u_int8_t *)(packet + sizeof(struct ether_header) + sizeof(struct ip6_hdr));
    payload_len = header->caplen - sizeof(struct ether_header) - sizeof(struct ip6_hdr);

    if (ipv6_header->ip6_nxt == IPPROTO_TCP || ipv6_header->ip6_nxt == IPPROTO_UDP)
    {
        printf("TTL: %d\n", ipv6_header->ip6_hops);
    }
}
