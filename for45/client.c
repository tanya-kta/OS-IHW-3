#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include "../info.h"


int main(int argc, char *argv[])
{
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr; /* Echo server address */
    unsigned short echoServPort;     /* Echo server port */
    char *servIP;                    /* Server IP address (dotted quad) */
    int buffer[MAX_INTS];
    int bytesRcvd;                   /* Bytes read in single recv() */

    if ((argc < 3) || (argc > 4))    /* Test for correct number of arguments */
    {
       fprintf(stderr, "Usage: %s <Decoder Path> <Server IP> [<Echo Port>]\n",
               argv[0]);
       exit(1);
    }

    servIP = argv[2];             /* First arg: server IP address (dotted quad) */

    if (argc == 4)
        echoServPort = atoi(argv[3]); /* Use given port, if any */
    else
        echoServPort = 7;  /* 7 is the well-known port for the echo service */

    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        dieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));     /* Zero out structure */
    echoServAddr.sin_family      = AF_INET;             /* Internet address family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);   /* Server IP address */
    echoServAddr.sin_port        = htons(echoServPort); /* Server port */

    /* Establish the connection to the echo server */
    if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0) {
        dieWithError("connect() failed");
    }

    if ((bytesRcvd = recv(sock, buffer, sizeof(buffer), 0)) < 0) {
        dieWithError("recv() failed");
    }

    int decoder[26];
    getDecoder(decoder, argv[1]);

    for(; bytesRcvd > 0;) {
        char decoded[MAX_INTS + 1];
        int i = 0;
        printf("%d\n", bytesRcvd);
        for (; i < bytesRcvd / 4; ++i) {
            printf("%d ", buffer[i]);
            decoded[i] = getCodedLetter(decoder, buffer[i]);
        }
        decoded[i] = '\0';
        if (send(sock, decoded, strlen(decoded), 0) != strlen(decoded))
            dieWithError("send() sent a different number of bytes than expected");
        sleep(2);
        if ((bytesRcvd = recv(sock, buffer, sizeof(buffer), 0)) < 0)
            dieWithError("recv() failed");
    }

    close(sock);
    exit(0);
}
