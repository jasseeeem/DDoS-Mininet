#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <netinet/ip6.h>
#include <net/ethernet.h>
#include <curl/curl.h>

#define MAX_PACKETS 1
#define INTERFACE "enp0s3"

int hlim;

// void process_curl_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);

void process_curl_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
    struct ip6_hdr *ipv6_header;
    u_int8_t *payload;
    u_int32_t payload_len;

    ipv6_header = (struct ip6_hdr *)(packet + sizeof(struct ether_header));
    payload = (u_int8_t *)(packet + sizeof(struct ether_header) + sizeof(struct ip6_hdr));
    payload_len = header->caplen - sizeof(struct ether_header) - sizeof(struct ip6_hdr);

    if (ipv6_header->ip6_nxt == IPPROTO_TCP || ipv6_header->ip6_nxt == IPPROTO_UDP)
    {
        printf("Hop Limit: %d\n", ipv6_header->ip6_hops);
        hlim = ipv6_header->ip6_hops;
    }
}

int curl_to_hlim(char src_ip[])
{
    CURL *curl;
    CURLcode res;

    // IP address to curl
    char ip_address[50];
    // printf("HERE %s\n", src_ip);
    sprintf(ip_address, "http://[%s]/", src_ip);

    curl = curl_easy_init();
    if (curl)
    {
        // Set the URL to the IP address to curl
        curl_easy_setopt(curl, CURLOPT_URL, ip_address);

        // Set the callback function for writing response data
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);

        // Set the response data to be written to stdout
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, stdout);

        // LISTEN FOR PACKETS FROM SOURCE
        char errbuf[PCAP_ERRBUF_SIZE];
        pcap_t *handle;
        struct bpf_program filter;
        char filter_exp[100];

        sprintf(filter_exp, "ip6 and src host %s", src_ip);
        handle = pcap_open_live(INTERFACE, BUFSIZ, 1, 1000, errbuf);
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

        printf("Listening for traffic from %s...\n", src_ip);

        res = curl_easy_perform(curl);
        pcap_loop(handle, MAX_PACKETS, process_curl_packet, NULL);

        // Perform the GET request

        // Get hlim from the return packet
        // int hlim = get_hlim_from_src(src_ip);

        // Ignoring errors as we assume return packet will be sent

        // if (res != CURLE_OK)
        // {
        //     fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        // }

        // Clean up
        curl_easy_cleanup(curl);
        pcap_close(handle);

        return hlim;
    }

    return -1;
}
