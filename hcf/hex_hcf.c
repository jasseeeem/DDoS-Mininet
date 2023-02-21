#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/ethernet.h>

#include "hcf_hdf5.c"

#define HLIM_BYTE_POSITION 21

#define MURMUR_C1 0xcc9e2d51
#define MURMUR_C2 0x1b873593
#define MURMUR_R1 15
#define MURMUR_R2 13
#define MURMUR_M 5
#define MURMUR_N 0xe6546b64

int hlim_to_hop_count(int);
void ProcessPacket(unsigned char *, int);

int sock_raw;
int i, j;
struct sockaddr_in6 source, dest;

// MurmurHash3_x86_32 hash function
uint32_t murmur_hash(const char *key, size_t len, uint32_t seed)
{
    uint32_t h = seed;
    const uint32_t *data = (const uint32_t *)key;
    const size_t nblocks = len / 4;
    for (size_t i = 0; i < nblocks; i++)
    {
        uint32_t k = data[i];
        k *= MURMUR_C1;
        k = (k << MURMUR_R1) | (k >> (32 - MURMUR_R1));
        k *= MURMUR_C2;
        h ^= k;
        h = (h << MURMUR_R2) | (h >> (32 - MURMUR_R2));
        h = h * MURMUR_M + MURMUR_N;
    }
    const uint8_t *tail = (const uint8_t *)(key + nblocks * 4);
    uint32_t k1 = 0;
    switch (len & 3)
    {
    case 3:
        k1 ^= tail[2] << 16;
    case 2:
        k1 ^= tail[1] << 8;
    case 1:
        k1 ^= tail[0];
        k1 *= MURMUR_C1;
        k1 = (k1 << MURMUR_R1) | (k1 >> (32 - MURMUR_R1));
        k1 *= MURMUR_C2;
        h ^= k1;
    }
    h ^= len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

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
    struct in6_addr ip_addr;
    uint32_t hash;
<<<<<<< HEAD
=======
    char hash_left_str[10],hash_right_str[10];
>>>>>>> 65bb994183ca6a8a44fcafd7e81c33c4f6730444

    snprintf(hlim_str, sizeof(hlim_str), "%02x", buffer[HLIM_BYTE_POSITION]);
    hlim = (int)strtol(hlim_str, NULL, 16);

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

    inet_pton(AF_INET6, src_ip, &ip_addr);
    hash = murmur_hash((const char *)&ip_addr, sizeof(ip_addr), 0);
    hash = (hash >> 8) & 0xffffff; // Take the least significant 24 bits

    printf("Hash value: %06x\n", hash);

    //new stuff
    uint32_t hash_left, hash_right;
    hash_left=(hash>>12);
    printf("Hash left: %02x\n",hash_left);

    hash_right=(hash& 0xfff);
    printf("Hash right: %02x\n",hash_right);
    
    sprintf( hash_left_str, "%02x", hash_left );
    sprintf( hash_right_str, "%02x", hash_right );
    // printf("Hash string: %s\n", hexstr);

    int hash_left_int, hash_right_int;
    hash_left_int=(int)strtol(hash_left_str,NULL,16);
    printf("Hash left int: %d\n", hash_left_int);
    hash_right_int=(int)strtol(hash_right_str,NULL,16);
    printf("Hash left int: %d\n", hash_right_int);

    if (update_hlim_value(hash_left_int, hash_right_int, hlim_to_hop_count(hlim))==0)
        printf("Hlim updated \n\n");

    //end of new stuff
}

int main()
{
    int data_size;
    struct sockaddr_in6 saddr;
    socklen_t saddr_size;
    struct in_addr in;
    main_();
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