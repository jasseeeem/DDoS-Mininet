#include <stdio.h>           //For standard things
#include <stdlib.h>          //malloc
#include <string.h>          //memset
#include <netinet/ip_icmp.h> //Provides declarations for icmp header
#include <netinet/udp.h>     //Provides declarations for udp header
#include <netinet/tcp.h>     //Provides declarations for tcp header
#include <netinet/ip.h>      //Provides declarations for ip header
#include <netinet/ip6.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <linux/types.h>
#include <net/ethernet.h>

void ProcessPacket(unsigned char *, int);
void print_ip6_header(unsigned char *, int);
void print_tcp_packet(unsigned char *, int);
void PrintData(unsigned char *, int);

int sock_raw;
FILE *logfile;
int tcp = 0, udp = 0, icmp = 0, others = 0, igmp = 0, total = 0, i, j;
struct sockaddr_in6 source, dest;

int main()
{
    int data_size;
    struct sockaddr_in6 saddr;
    socklen_t saddr_size;
    struct in_addr in;

    logfile = fopen("hcf.log", "w");
    if (logfile == NULL)
    {
        printf("Unable to create file.");
    }

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

void ProcessPacket(unsigned char *buffer, int size)
{
    int i, count = 0;
    char hlim_str[3] = {0};
    for (i = 0; i < size; i++)
    {
        count++;
        if (count == 22)
        {
            int hlim;
            snprintf(hlim_str, sizeof(hlim_str), "%02x", buffer[i]);
            hlim = (int)strtol(hlim_str, NULL, 16);
            printf("hlim: %d\n", hlim);
            printf("Source Address: ");
        }
        if (count >= 23 && count <= 39)
        {
            printf("%02x", buffer[i]);
        }
    }
    printf("\n\n");
}
