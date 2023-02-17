#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *fp;
    char output[1024];

    // Replace "ls" with the command you want to run
    fp = popen("sudo tcpdump -nn -vv 'ip6' | awk '{for (i=1;i<NF;i++) if ($i == 'hlim') print $(i+1) $(i+8) }'", "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }

    while (fgets(output, sizeof(output), fp) != NULL) {
        printf("%s", output);
    }

    pclose(fp);
    return 0;
}