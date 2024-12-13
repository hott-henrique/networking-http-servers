#include "techniques.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <pthread.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#include "webserver.h"
#include "error.h"


void iterative(int sockfd) {
    struct sockaddr_in client;
    socklen_t clientsz = sizeof(client);

    while (1) {
        int connfd = accept(sockfd, (struct sockaddr *) &(client), &clientsz);

        if (connfd < 0) {
            error("Failed: server accept\n");
        }

        webserver(connfd);
    }
}

void forking(int sockfd) {
    struct sockaddr_in client;
    socklen_t clientsz = sizeof(client);

    while (1) {
        int connfd = accept(sockfd, (struct sockaddr *) &(client), &clientsz);

        if (connfd < 0) {
            error("Failed: server accept\n");
        }

        pid_t pid = fork();

        if (pid == -1) {
            close(connfd);
            error("Fork Technique ERROR: Failed to create child process.");
        }

        if (pid == 0) {
            webserver(connfd);
            exit(EXIT_SUCCESS);
        } else {
            close(connfd);
        }
    }
}

struct pc_data_t { int * tasks; pthread_mutex_t mutex; };

void * producer_consumer_CONSUMER(void * d) {
    struct pc_data_t * pc_data = (struct pc_data_t *) d;

    while (1) {
        pthread_mutex_lock(&pc_data->mutex);

        if (arrlen(pc_data->tasks) != 0) {
            int connfd = arrpop(pc_data->tasks);

            pthread_mutex_unlock(&pc_data->mutex);

            webserver(connfd);
        } else {
            pthread_mutex_unlock(&pc_data->mutex);
        }
    }

    return NULL;
}

void producer_consumer_PRODUCER(int sockfd, struct pc_data_t * pc_data) {
    struct sockaddr_in client;
    socklen_t clientsz = sizeof(client);

    while (1) {
        int connfd = accept(sockfd, (struct sockaddr *) &(client), &clientsz);

        if (connfd < 0) {
            error("Failed: server accept\n");
        }

        pthread_mutex_lock(&pc_data->mutex);

        arrpush(pc_data->tasks, connfd);

        pthread_mutex_unlock(&pc_data->mutex);
    }
}

void producer_consumer(int sockfd) {
    struct pc_data_t  pc_data = { .tasks = NULL };

    pthread_mutex_init(&(pc_data.mutex), NULL);

    const int MAX_THREADS = 4;
    pthread_t threads[MAX_THREADS];

    for (int i = 0; i < MAX_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, producer_consumer_CONSUMER, (void*) &pc_data) != 0) {
            close(sockfd);
            error("Producer-Consumer technique error: Failed to create thread.");
        }
    }

    producer_consumer_PRODUCER(sockfd, &pc_data);
}

void concurrent(int sockfd) {
    struct sockaddr_in client;
    socklen_t clientsz = sizeof(client);

    fd_set master;
    FD_ZERO(&master);

    fd_set read_fds;
    FD_ZERO(&read_fds);

    FD_SET(sockfd, &master);
    int fd_max = sockfd;

    while (1) {
        read_fds = master;

        if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) == -1) {
            error("Concurrent technique error: select failed.");
        }

        for (int i = 0; i <= fd_max; i++) {
            if (!FD_ISSET(i, &read_fds)) {
                continue;
            }

            if (i == sockfd) {
                clientsz = sizeof(client);

                int connfd = accept(sockfd, (struct sockaddr *) &client, &clientsz);

                if (connfd < 0){
                    error("Failed: server accept\n");
                }

                FD_SET(connfd, &master);

                if (connfd > fd_max) {
                    fd_max = connfd;
                }
            } else {
                webserver(i);
                FD_CLR(i, &master);
            }
        }
    }
}
