#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

int main() {

    char addr_str[INET6_ADDRSTRLEN];

    char ip_str[40];
    // char ip_str[40] = "fe80::215:5dff:fe0b:811c";
    printf("Enter a valid IPv6 address: ");
    fgets(ip_str, 40, stdin);
    ip_str[strcspn(ip_str, "\n")] = 0;
    struct in6_addr result;

    if (inet_pton(AF_INET6, ip_str, &result) != 1) // success!
    {
        //failed, perhaps not a valid representation of IPv6?
        printf("NOT OK");
        return 1;
    }

    // Convert the IPv6 address to a string
    inet_ntop(AF_INET6, &result, addr_str, INET6_ADDRSTRLEN);
    printf("Full IPv6 address: %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n",
           ntohs(result.s6_addr16[0]), ntohs(result.s6_addr16[1]),
           ntohs(result.s6_addr16[2]), ntohs(result.s6_addr16[3]),
           ntohs(result.s6_addr16[4]), ntohs(result.s6_addr16[5]),
           ntohs(result.s6_addr16[6]), ntohs(result.s6_addr16[7]));

    return 0;
}