#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <netinet/in.h>

#include <signal.h>
#include <unistd.h>

#include "error.h"
#include "techniques.h"


int sockfd;

void handle_signal(int signal) {
    puts("\nTerminating application...");
    if (sockfd >= 0) {
        close(sockfd);
    }
    exit(0);
}

int main(int argc, char * argv[]) {
    if (argc < 2) {
        printf("Usage: %s METHOD\n", argv[0]);
        puts(
            "1: Iterative\n"
            "2: Forking\n"
            "3: Producer-Consumer\n"
            "4: Concurrent with Select\n"
        );
        exit(1);
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }

    struct sockaddr_in server_address;

    memset((char *) &server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8080);

    int enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    if (bind(sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        error("ERROR on binding");
    }

    puts("Server running on http://localhost:8080");

    listen(sockfd, 0);

    int method = atoi(argv[1]);

    switch (method) {
        case 1:
            iterative(sockfd);
        break;

        case 2:
            forking(sockfd);
        break;

        case 3:
            producer_consumer(sockfd);
        break;

        case 4:
            concurrent(sockfd);
        break;

        default:
            error("ERROR: Unknow technique selected.");
    }

    close(sockfd);

    return 0;
}
