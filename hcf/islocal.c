#include <ifaddrs.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int is_local_address(struct in6_addr *addr)
{
    struct ifaddrs *ifaddr, *ifa;

    // get a list of the device's network interfaces
    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return 0;
    }

    // iterate over the list of network interfaces
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        // skip interfaces that are not up or not IPv6
        if (ifa->ifa_flags & IFF_UP && ifa->ifa_addr->sa_family == AF_INET6)
        {
            // get the IPv6 address associated with this interface
            struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)ifa->ifa_addr;

            // check if the address matches
            if (memcmp(addr, &sa6->sin6_addr, sizeof(struct in6_addr)) == 0)
            {
                // the address matches, so it is a local address
                freeifaddrs(ifaddr);
                return 1;
            }
        }
    }

    // the address does not match any local addresses
    freeifaddrs(ifaddr);
    return 0;
}

int main()
{
    struct in6_addr addr;

    // initialize the IPv6 address
    inet_pton(AF_INET6, "fe80::", &addr);

    // check if the address belongs to the device
    if (is_local_address(&addr))
    {
        printf("IPv6 address is local\n");
    }
    else
    {
        printf("IPv6 address is not local\n");
    }

    return 0;
}
