#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/ethernet.h>

#include "hdf5.c"
#include "murmur_hash.c"

#define HLIM_BYTE_POSITION 21
#define NEXT_HEADER_BYTE_POSITION 20
#define IP_V_BYTE_POSITION 14

// int hlim_to_hop_count(int);
// void process_packet(unsigned char *, int);

int sock_raw;
int i, j;
struct sockaddr_in6 source, dest;

int starts_with(const char *str, const char *prefix)
{
    size_t prefix_len = strlen(prefix);
    return strncmp(str, prefix, prefix_len) == 0;
}

int is_ip_routable(char src_ip[])
{
    if (starts_with(src_ip, "fe80"))
        return 0;
    else if (starts_with(src_ip, "fc00"))
        return 0;
    else if (starts_with(src_ip, "fd00"))
        return 0;
    else if (starts_with(src_ip, "2001:0db8"))
        return 0;
    else if (starts_with(src_ip, "2001:0010"))
        return 0;
    else if (starts_with(src_ip, "2002"))
        return 0;
    else if (starts_with(src_ip, "3ffe"))
        return 0;
    else if (starts_with(src_ip, "ff00"))
        return 0;
    else if (strcmp(src_ip, "0000:0000:0000:0000:0000:0000:0000:0000") == 0)
        return 0;
    else if (strcmp(src_ip, "0000:0000:0000:0000:0000:0000:0000:0001") == 0)
        return 0;
    else
        return 1;
}

void process_packet(unsigned char *buffer, int size)
{
    int i, hlim, ip_v, protocol;
    char hlim_str[3] = {0}, src_ip[40], ip_v_str[3], protocol_str[3];
    struct in6_addr ip_addr;
    uint32_t hash;
    char hash_left_str[10], hash_right_str[10];

    snprintf(hlim_str, sizeof(hlim_str), "%02x", buffer[HLIM_BYTE_POSITION]);
    hlim = (int)strtol(hlim_str, NULL, 16);

    snprintf(ip_v_str, sizeof(ip_v_str), "%02x", buffer[IP_V_BYTE_POSITION]);
    ip_v = (int)strtol(ip_v_str, NULL, 16) >> 4;

    snprintf(protocol_str, sizeof(protocol_str), "%02x", buffer[NEXT_HEADER_BYTE_POSITION]);
    protocol = (int)strtol(protocol_str, NULL, 16);

    snprintf(src_ip,
             40,
             "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
             buffer[HLIM_BYTE_POSITION + 1],
             buffer[HLIM_BYTE_POSITION + 2],
             buffer[HLIM_BYTE_POSITION + 3],
             buffer[HLIM_BYTE_POSITION + 4],
             buffer[HLIM_BYTE_POSITION + 5],
             buffer[HLIM_BYTE_POSITION + 6],
             buffer[HLIM_BYTE_POSITION + 7],
             buffer[HLIM_BYTE_POSITION + 8],
             buffer[HLIM_BYTE_POSITION + 9],
             buffer[HLIM_BYTE_POSITION + 10],
             buffer[HLIM_BYTE_POSITION + 11],
             buffer[HLIM_BYTE_POSITION + 12],
             buffer[HLIM_BYTE_POSITION + 13],
             buffer[HLIM_BYTE_POSITION + 14],
             buffer[HLIM_BYTE_POSITION + 15],
             buffer[HLIM_BYTE_POSITION + 16]);
    printf("Source Address: %s\n", src_ip);
    printf("IPV: %d\nProtocol: %d\n", ip_v, protocol);

    inet_pton(AF_INET6, src_ip, &ip_addr);
    hash = murmur_hash((const char *)&ip_addr, sizeof(ip_addr), 0);
    hash = (hash >> 8) & 0xffffff; // Take the least significant 24 bits

    // printf("Hash value: %06x\n", hash);

    // new stuff
    uint32_t hash_left, hash_right;
    hash_left = (hash >> 12);
    // printf("Hash left: %02x\n", hash_left);

    hash_right = (hash & 0xfff);
    // printf("Hash right: %02x\n", hash_right);

    sprintf(hash_left_str, "%02x", hash_left);
    sprintf(hash_right_str, "%02x", hash_right);
    // printf("Hash string: %s\n", hexstr);

    int hash_left_int, hash_right_int;
    hash_left_int = (int)strtol(hash_left_str, NULL, 16);
    // printf("Hash left int: %d\n", hash_left_int);
    hash_right_int = (int)strtol(hash_right_str, NULL, 16);
    // printf("Hash right int: %d\n", hash_right_int);

    if (ip_v != 6)
    {
        printf("⛔ Not an IPv6 packet\n\n");
    }
    else if (protocol != 6)
    {
        printf("⛔ Not a TCP packet\n\n");
    }
    else if (is_ip_routable(src_ip) != 1)
    {
        printf("⛔ IP is not routable\n\n");
    }
    else
    {
        if (check_hop_count(src_ip, hash_left_int, hash_right_int, hlim) == 0)
        {
            printf("✅ Hop Count checked\n\n");
        }
        else
        {
            printf("❌ Hop Count not checked\n\n");
        }
    }
}

int main()
{
    int data_size;
    struct sockaddr_in6 saddr;
    socklen_t saddr_size;
    struct in_addr in;

    hid_t file;

    printf("Starting HCF...\n");
    get_or_create_table();

    sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IPV6));
    if (sock_raw < 0)
    {
        printf("Socket Error\n");
        return 1;
    }

    unsigned char *buffer = (unsigned char *)malloc(65536);
    while (1)
    {
        saddr_size = sizeof(saddr);
        // Receive a packet
        printf("Receiving a packet\n");
        data_size = recvfrom(sock_raw, buffer, 65536, 0, (struct sockaddr *)&saddr, &saddr_size);
        printf("Received a packet\n");
        if (data_size < 0)
        {
            printf("Recvfrom error , failed to get packets\n");
            return 1;
        }
        // Now process the packet
        process_packet(buffer, data_size);

        // NEED A WAY TO TERMINATE THE LOOP
        // NEED A WAY TO CLOSE THE H5 FILE POINTER AFTER PROGRAM TERMINATION
    }
    close(sock_raw);
    printf("Finished\n");
    return 0;
}