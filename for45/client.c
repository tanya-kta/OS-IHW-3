#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include "../clientInfo.h"


int main(int argc, char *argv[])
{
    int sock;                          /* Socket descriptor */
    struct sockaddr_in echo_serv_addr; /* Echo server address */
    unsigned short echo_serv_port;     /* Echo server port */
    char *serv_ip;                     /* Server IP address (dotted quad) */
    int buffer[MAX_INTS];
    int bytes_rcvd;                    /* Bytes read in single recv() */

    if ((argc < 3) || (argc > 4)) {
       fprintf(stderr, "Usage: %s <Decoder Path> <Server IP> [<Echo Port>]\n",
               argv[0]);
       exit(1);
    }

    serv_ip = argv[2];

    if (argc == 4) {
       echo_serv_port = atoi(argv[3]); /* Use given port, if any */
    } else {
        echo_serv_port = 7; /* 7 is the well-known port for the echo service */
    }

    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        dieWithError("socket() failed");
    }

    /* Construct the server address structure */
    memset(&echo_serv_addr, 0, sizeof(echo_serv_addr));     /* Zero out structure */
    echo_serv_addr.sin_family      = AF_INET;             /* Internet address family */
    echo_serv_addr.sin_addr.s_addr = inet_addr(serv_ip);   /* Server IP address */
    echo_serv_addr.sin_port        = htons(echo_serv_port); /* Server port */

    /* Establish the connection to the echo server */
    if (connect(sock, (struct sockaddr *) &echo_serv_addr, sizeof(echo_serv_addr)) < 0) {
        dieWithError("connect() failed");
    }

    if ((bytes_rcvd = recv(sock, buffer, sizeof(buffer), 0)) < 0) {
        dieWithError("recv() failed");
    }

    int decoder[26];
    getDecoder(decoder, argv[1]);

    for(; bytes_rcvd > 0;) {
        char decoded[MAX_INTS + 1];
        int i = 0;
        printf("%d\n", bytes_rcvd);
        for (; i < bytes_rcvd / 4; ++i) {
            printf("%d ", buffer[i]);
            decoded[i] = getCodedLetter(decoder, buffer[i]);
        }
        printf("\n");
        decoded[i] = '\0';
        if (send(sock, decoded, strlen(decoded), 0) != strlen(decoded)) {
            dieWithError("send() sent a different number of bytes than expected");
        }
        sleep(2);
        if ((bytes_rcvd = recv(sock, buffer, sizeof(buffer), 0)) < 0) {
            dieWithError("recv() failed");
        }
    }

    close(sock);
    exit(0);
}
