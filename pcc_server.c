#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#define BUF_LEN 1024 * 1024 - 1
#define REDABLE_MIN 32
#define REDABLE_MAX 126
#define LISTEN_QUEUE_SIZE 10

// those vars are static for them to be accessible via sighandler
u_int32_t pcc_total[REDABLE_MAX - REDABLE_MIN + 1];
int accepting_connections = 1;
int connfd = -1;

void display_stats_sighandler(int signum);

void display_stats_sighandler(int signum)
{
    accepting_connections = 0;
    while (connfd != -1)
    {
    } // wait for current client handling.

    for (int i = 0; i <= REDABLE_MAX - REDABLE_MIN; i++)
    {
        printf("char '%c' : %u times\n", i + REDABLE_MIN, pcc_total[i]);
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    char data_buff[BUF_LEN + 1];
    memset(pcc_total, 0, sizeof(pcc_total));

    if (argc != 2)
    {
        perror("Invalid number of arguments passed");
        exit(1);
    }

    struct sigaction display_stats_sigint_action = {
        .sa_handler = display_stats_sighandler,
        .sa_flags = SA_RESTART};

    if (sigaction(SIGINT, &display_stats_sigint_action, NULL) == -1)
    {
        perror("Failed registering new sigaction");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    socklen_t addrsize = sizeof(serv_addr);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
    {
        perror("Failed initializing socket\n");
        exit(1);
    }

    memset(&serv_addr, 0, addrsize);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (0 != bind(listenfd, (struct sockaddr *)&serv_addr, addrsize))
    {
        printf("\n Error : Bind Failed. %s \n", strerror(errno));
        exit(1);
    }

    if (0 != listen(listenfd, LISTEN_QUEUE_SIZE))
    {
        perror("\n Error : Listen Failed. %s \n");
        exit(1);
    }

    while (accepting_connections)
    {
        // Accept a connection.
        connfd = accept(listenfd, NULL, NULL);
        if (connfd < 0)
        {
            perror("\n Error : Accept Failed. %s \n");
            exit(1);
        }

        // ----- Read N from client
        uint32_t N;
        ssize_t total_recieved = 0;
        ssize_t not_recieved = sizeof(uint32_t);

        while (not_recieved > 0)
        {
            ssize_t bytes_read = read(connfd, &N + total_recieved, not_recieved);
            if (bytes_read <= 0 && (bytes_read == 0 || errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)) // We were told to assume those are the only possible errors.
            {
                perror("Client connection terminated unexpectedly\n");
                close(connfd);
                connfd = -1;
            }
            else
            {
                total_recieved += bytes_read;
                not_recieved -= bytes_read;
            }
            total_recieved += bytes_read;
            not_recieved -= bytes_read;
        }
        if (connfd == -1)
        {
            continue;
        }

        N = ntohl(N);

        // ------ Read DATA from client and update the char counts
        total_recieved = 0;
        not_recieved = N;
        uint32_t C = 0;
        while (not_recieved > 0)
        {
            memset(data_buff, 0, BUF_LEN);
            ssize_t bytes_read = read(connfd, &data_buff, not_recieved);
            if (bytes_read <= 0 && (bytes_read == 0 || errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)) // We were told to assume those are the only possible errors.
            {
                perror("Client connection terminated unexpectedly\n");
                close(connfd);
                connfd = -1;
                not_recieved = 0;
            }
            else
            {
                for (int i = 0; i < N; i++)
                {
                    if (data_buff[i] <= REDABLE_MAX && data_buff[i] >= REDABLE_MIN)
                    {
                        pcc_total[(data_buff[i] + 0) - REDABLE_MIN] += 1;
                        C++;
                    }
                }
                total_recieved += bytes_read;
                not_recieved -= bytes_read;
            }
        }

        if (connfd == -1)
        {
            continue;
        }

        // ------ Send C to client
        int32_t conv = htonl(C);
        char *data = (char *)&conv;
        ssize_t notwritten = sizeof(uint32_t);

        while (notwritten > 0)
        {
            ssize_t nsent = write(connfd, data, notwritten);
            if (nsent <= 0 && (nsent == 0 || errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)) // We were told to assume those are the only possible errors.
            {
                perror("Client connection terminated unexpectedly\n");
                close(connfd);
                connfd = -1;
            }
            else
            {
                data += nsent;
                notwritten -= nsent;
            }
        }

        // close socket
        connfd = -1;
        close(connfd);
    }
}
