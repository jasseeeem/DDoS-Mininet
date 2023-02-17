#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MURMUR_C1 0xcc9e2d51
#define MURMUR_C2 0x1b873593
#define MURMUR_R1 15
#define MURMUR_R2 13
#define MURMUR_M 5
#define MURMUR_N 0xe6546b64

// MurmurHash3_x86_32 hash function
uint32_t murmur_hash(const char *key, size_t len, uint32_t seed) {
    uint32_t h = seed;
    const uint32_t *data = (const uint32_t *) key;
    const size_t nblocks = len / 4;
    for (size_t i = 0; i < nblocks; i++) {
        uint32_t k = data[i];
        k *= MURMUR_C1;
        k = (k << MURMUR_R1) | (k >> (32 - MURMUR_R1));
        k *= MURMUR_C2;
        h ^= k;
        h = (h << MURMUR_R2) | (h >> (32 - MURMUR_R2));
        h = h * MURMUR_M + MURMUR_N;
    }
    const uint8_t *tail = (const uint8_t *) (key + nblocks * 4);
    uint32_t k1 = 0;
    switch (len & 3) {
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

int main() {
    char ip_str[40];
    uint32_t h;

    printf("Enter an IPv6 address: ");
    fgets(ip_str, sizeof(ip_str), stdin);

    // Convert IPv6 address to binary
    struct in6_addr ip;
    if (inet_pton(AF_INET6, ip_str, &ip) != 1) {
        printf("Invalid IPv6 address\n");
        return 1;
    }

    // Hash the IPv6 address to 24 bits
    h = murmur_hash((const char *) &ip, sizeof(ip), 0);
    h = (h >> 8) & 0xffffff; // Take the least significant 24 bits

    printf("Hash value: %06x\n", h);

    return 0;
}
