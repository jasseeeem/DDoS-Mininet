#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <arpa/inet.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <net/ethernet.h>

void handle_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);

int main(int argc, char *argv[])
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;
    struct bpf_program fp;
    bpf_u_int32 mask;
    bpf_u_int32 net;

    if (pcap_lookupnet("eth0", &net, &mask, errbuf) == -1) {
        fprintf(stderr, "Can't get netmask for device eth0\n");
        net = 0;
        mask = 0;
    }

    handle = pcap_open_live("eth0", BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Couldn't open device eth0: %s\n", errbuf);
        return 2;
    }

    if (pcap_compile(handle, &fp, "tcp6", 0, net) == -1) {
        fprintf(stderr, "Couldn't compile filter\n");
        return 2;
    }

    if (pcap_setfilter(handle, &fp) == -1) {
        fprintf(stderr, "Couldn't set filter\n");
        return 2;
    }

    pcap_loop(handle, -1, handle_packet, NULL);

    pcap_close(handle);

    return 0;
}

void handle_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
    const struct ip6_hdr *ip;
    const struct tcphdr *tcp;
    u_int ip_len;

    ip = (struct ip6_hdr*)(packet + sizeof(struct ether_header));
    ip_len = ntohs(ip->ip6_plen);

    if (ip->ip6_nxt == IPPROTO_TCP) {
    // struct in6_addr ipv6_addr;
    //     char ipv6_str[INET6_ADDRSTRLEN];
    //     if (inet_ntop(AF_INET6, &ipv6_addr, ipv6_str, INET6_ADDRSTRLEN) == NULL) {
    //         perror("inet_ntop");
    //         exit(EXIT_FAILURE);
    //     }

    //     printf("IPv6 address: %s\n", ipv6_str);

        tcp = (struct tcphdr*)(packet + sizeof(struct ether_header) + sizeof(struct ip6_hdr));
        // printf("Source IP: %s\n", inet_ntop(ip->ip6_src));
        // printf("Destination IP: %s\n", inet_ntop(ip->ip6_dst));
        printf("Source Port: %d\n", ntohs(tcp->th_sport));
        printf("Destination Port: %d\n", ntohs(tcp->th_dport));
        printf("Sequence Number: %u\n", ntohl(tcp->th_seq));
        printf("Acknowledge Number: %u\n", ntohl(tcp->th_ack));
        printf("Header Length: %d\n", tcp->th_off * 4);
        printf("Flags:");
        if (tcp->th_flags & TH_FIN)
            printf(" FIN");
        if (tcp->th_flags & TH_SYN)
            printf(" SYN");
        if (tcp->th_flags & TH_RST)
            printf(" RST");
        if (tcp->th_flags & TH_PUSH)
            printf(" PUSH");
        if (tcp->th_flags & TH_ACK)
            printf(" ACK");
        if (tcp->th_flags & TH_URG)
            printf(" URG");
        printf("\n\n");
    }
}