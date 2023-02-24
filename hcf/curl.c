#include <stdio.h>
#include <curl/curl.h>

int main(void)
{
    CURL *curl;
    CURLcode res;

    // IP address to curl
    char *ip_address = "http://[2404:6800:4007:0821:0000:0000:0000:2003]";

    curl = curl_easy_init();
    if (curl)
    {
        // Set the URL to the IP address to curl
        curl_easy_setopt(curl, CURLOPT_URL, ip_address);

        // Set the callback function for writing response data
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);

        // Set the response data to be written to stdout
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, stdout);

        // Perform the GET request
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        // Clean up
        curl_easy_cleanup(curl);
    }

    return 0;
}
