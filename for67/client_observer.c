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
    char buffer[10010];
    int bytes_rcvd;                    /* Bytes read in single recv() */

    if ((argc < 3) || (argc > 4)) {
        fprintf(stderr, "Usage: %s <Server IP> [<Echo Port>]\n",
                argv[0]);
        exit(1);
    }

    serv_ip = argv[1];

    if (argc == 3) {
        echo_serv_port = atoi(argv[2]); /* Use given port, if any */
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

    if ((bytes_rcvd = recv(sock, buffer, sizeof(buffer) - 1, 0)) < 0) {
        dieWithError("recv() failed");
    }

    for(; bytes_rcvd > 0;) {
        printf("Received: ");
        buffer[bytes_rcvd] = '\0';
        printf("%s\n", buffer);
        sleep(2);
        if ((bytes_rcvd = recv(sock, buffer, sizeof(buffer) - 1, 0)) < 0)
            dieWithError("recv() failed");
    }

    close(sock);
    exit(0);
}
