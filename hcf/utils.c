#include <stdio.h>
#include <stdlib.h>


void main()
{
    __uint32_t hash = 0xABCDEF;
    printf("%X\n",hash);

    __uint16_t a = hash >> 12;
    printf("%X\n",a);

    __uint16_t b = hash;
    b = hash << 4;
    printf("%X\n",b);
}