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

#define PORT 3000
#define BUF_LEN 1024 * 1024 - 1
#define REDABLE_MIN 32
#define REDABLE_MAX 126

int main(int argc, char *argv[])
{
    int pcc_total[REDABLE_MAX - REDABLE_MIN + 1];
    char data_buff[BUF_LEN + 1];
    const int enable = 1;
    if (argc != 2)
    {
        perror("Invalid number of arguments passed");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    memset(&serv_addr, 0, addrsize);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    if (0 != bind(listenfd, (struct sockaddr *)&serv_addr, addrsize))
    {
        printf("\n Error : Bind Failed. %s \n", strerror(errno));
        return 1;
    }

    if (0 != listen(listenfd, 10))
    {
        printf("\n Error : Listen Failed. %s \n", strerror(errno));
        return 1;
    }

    while (1)
    {
        printf("listening\n");
        // Accept a connection.
        int connfd = accept(listenfd, NULL, NULL);
        printf("hello\n");
        if (connfd < 0)
        {
            printf("\n Error : Accept Failed. %s \n", strerror(errno));
            return 1;
        }

        // Read N from client
        uint32_t N;
        ssize_t total_recieved = 0;
        ssize_t not_recieved = sizeof(uint32_t);

        while (not_recieved > 0)
        {
            ssize_t bytes_read = read(listenfd, &N + total_recieved, not_recieved);
            // TODO error handling
            total_recieved += bytes_read;
            not_recieved -= bytes_read;
        }
        N = ntohl(N);
        printf("Read N\n");
        
        // Read DATA from client and update the char counts
        total_recieved = 0;
        not_recieved = N;

        while (not_recieved > 0)
        {
            ssize_t bytes_read = read(listenfd, &data_buff + total_recieved, not_recieved);
            // TODO error handling
            total_recieved += bytes_read;
            not_recieved -= bytes_read;
        }
        uint32_t C = 0;
        data_buff[total_recieved + 1] = '\0';
        for (int i = 0; i < N; i++)
        {
            if (data_buff[i] <= REDABLE_MAX && data_buff[i] >= REDABLE_MIN)
            {
                pcc_total[data_buff[i] + 0] += 1;
                C++;
            }
        }
        printf("read data\n");
        // Send C to client
        ssize_t total_sent = 0;
        ssize_t notwritten = sizeof(uint32_t);

        // keep looping until nothing left to write
        while (notwritten > 0)
        {
            ssize_t nsent = write(connfd, &C + total_sent, notwritten);
            // check if error occured (client closed connection?)
            assert(nsent >= 0);

            total_sent += nsent;
            notwritten -= nsent;
        }

        // close socket
        close(connfd);
    }
}
