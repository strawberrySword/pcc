#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// I subtract 1 here and add 1 at data_buffer initialization. this may seem weird but it is done so i can enforce last bit \0
#define BUF_LEN 1024 * 1024 - 1

int main(int argc, char *argv[])
{
    struct stat st;
    int sockfd = -1, datafd;
    char data_buffer[BUF_LEN + 1];

    if (argc != 4)
    {
        perror("Invalid number of arguments passed");
        exit(1);
    }

    // Assume server address and port are valid.
    // open file:
    if ((datafd = open(argv[3], O_RDONLY)) < 0)
    {
        perror("Cannot open file. please ensure file name is valid and file is accessible");
        exit(1);
    }
    // find N: (without file functions)
    uint32_t N;
    // N = lseek(datafd, 0, SEEK_END); // I wasnt sure if lseek was allowd so i used the stat method to get the file size.
    if (stat(argv[3], &st) == 0)
    {
        N = st.st_size;
    }
    else
    {
        perror("stat");
        exit(1);
    }

    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));
    serv_addr.sin_addr.s_addr = inet_addr(argv[2]);

    // connect socket to the target address
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\n Error : Connect Failed. %s \n", strerror(errno));
        return 1;
    }

    // ---- write N to server
    int32_t conv = htonl(N);
    char *data = (char *)&conv;
    int left = sizeof(conv);
    int ofs;
    while (left > 0)
    {
        if ((ofs = write(sockfd, data, left)) <= 0)
        {
            perror("Failed wrinting to server\n");
            close(sockfd);
            exit(1);
        }
        else
        {
            data += ofs;
            left -= ofs;
        }
    }

    // ---- write DATA to server
    ssize_t totalsent = 0;
    ssize_t notwritten = N;
    int size;
    // keep looping until nothing left to write
    while (notwritten > 0)
    {
        // copy BUF_LEN bytes from file to buffer:
        size = read(datafd, data_buffer + totalsent, BUF_LEN);
        data_buffer[size + 1] = '\0'; /*to make sure*/
        ssize_t nsent = write(sockfd, data_buffer + totalsent, size);
        if (nsent <= 0)
        {
            perror("Failed sending data to server \n");
            close(sockfd);
            exit(1);
        }

        totalsent += nsent;
        notwritten -= nsent;
    }
    close(datafd);
    // ---- read C from server
    uint32_t C;
    ssize_t total_recieved = 0;
    ssize_t not_recieved = sizeof(uint32_t);

    while (not_recieved > 0)
    {
        ssize_t bytes_read = read(sockfd, &C + total_recieved, not_recieved);
        if (bytes_read <= 0)
        {
            perror("Failed reading data from server \n");
            close(sockfd);
            exit(1);
        }
        else
        {
            total_recieved += bytes_read;
            not_recieved -= bytes_read;
        }
    }
    C = ntohl(C);

    close(sockfd);
    exit(0);
}
