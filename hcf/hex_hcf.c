#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/ethernet.h>

#define HLIM_BYTE_POSITION 21


int hlim_to_hop_count(int);
void ProcessPacket(unsigned char *, int);


int sock_raw;
int i, j;
struct sockaddr_in6 source, dest;


int hlim_to_hop_count(int hlim)
{
    if (hlim > 255 || hlim < 0)
        return -1;
    else if (hlim <= 64)
        return 64 - hlim;
    else if (hlim <= 128)
        return 128 - hlim;
    else
        return 255 - hlim;
}



void ProcessPacket(unsigned char *buffer, int size)
{
    int i, hlim;
    char hlim_str[3] = {0}, src_ip[40];

    snprintf(hlim_str, sizeof(hlim_str), "%02x", buffer[HLIM_BYTE_POSITION]);
    hlim = (int)strtol(hlim_str, NULL, 16);
    printf("Hop Count: %d\n", hlim_to_hop_count(hlim));

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
    printf("Source Address: %s\n\n", src_ip);
}



int main()
{
    int data_size;
    struct sockaddr_in6 saddr;
    socklen_t saddr_size;
    struct in_addr in;

    printf("Starting...\n");
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
        data_size = recvfrom(sock_raw, buffer, 65536, 0, (struct sockaddr *)&saddr, &saddr_size);
        if (data_size < 0)
        {
            printf("Recvfrom error , failed to get packets\n");
            return 1;
        }
        // Now process the packet
        ProcessPacket(buffer, data_size);

        // NEED A WAY TO TERMINATE THE LOOP
    }
    close(sock_raw);
    printf("Finished");
    return 0;
}