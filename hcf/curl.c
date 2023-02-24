#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <netinet/ip6.h>

struct MemoryStruct
{
    char *memory;
    size_t size;
};

void print_hex(unsigned char *buffer, size_t size)
{
    size_t i, j;
    for (i = 0; i < size; i += 16)
    {
        printf("%08lx:", i);
        for (j = 0; j < 16; j++)
        {
            if (i + j < size)
            {
                printf(" %02x", buffer[i + j]);
            }
            else
            {
                printf("   ");
            }
        }
        printf(" ");
        for (j = 0; j < 16; j++)
        {
            if (i + j < size)
            {
                if (buffer[i + j] >= 32 && buffer[i + j] <= 126)
                {
                    printf("%c", buffer[i + j]);
                }
                else
                {
                    printf(".");
                }
            }
            else
            {
                printf(" ");
            }
        }
        printf("\n");
    }
}

static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    struct MemoryStruct *mem = (struct MemoryStruct *)userdata;
    size_t num_bytes = size * nmemb;

    mem->memory = realloc(mem->memory, mem->size + num_bytes);
    if (mem->memory == NULL)
    {
        fprintf(stderr, "Out of memory\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), ptr, num_bytes);
    mem->size += num_bytes;

    return num_bytes;
}

int main(void)
{
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk = {NULL, 0};
    struct ip6_hdr *header;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, "http://[2404:6800:4007:821::2003]/");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V6);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        else
        {
            header = (struct ip6_hdr *)chunk.memory;
            printf("Received %ld bytes\n", chunk.size);
            printf("IPv6 Header:\n");
            print_hex((unsigned char *)header, sizeof(struct ip6_hdr));
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    free(chunk.memory);

    return 0;
}
