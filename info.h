#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <fcntl.h>

#define MAX_INTS        30    // максимальная длина текстового сообщения
#define MAXPENDING      5     /* Maximum outstanding connection requests */


void dieWithError(char *error_message) {
    perror(error_message);
    exit(1);
}

int createTcpServerSocket(unsigned short port) {
    int sock;                        /* socket to create */
    struct sockaddr_in echo_serv_addr; /* Local address */

    /* Create socket for incoming connections */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        dieWithError("socket() failed");
    }

    /* Construct local address structure */
    memset(&echo_serv_addr, 0, sizeof(echo_serv_addr));   /* Zero out structure */
    echo_serv_addr.sin_family = AF_INET;                /* Internet address family */
    echo_serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echo_serv_addr.sin_port = htons(port);              /* Local port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *) &echo_serv_addr, sizeof(echo_serv_addr)) < 0) {
        dieWithError("bind() failed");
    }

    /* Mark the socket so it will listen for incoming connections */
    if (listen(sock, MAXPENDING) < 0) {
        dieWithError("listen() failed");
    }

    return sock;
}

int acceptTcpConnection(int serv_sock) {
    int clnt_sock;                    /* Socket descriptor for client */
    struct sockaddr_in echo_clnt_addr; /* Client address */
    unsigned int clnt_len;            /* Length of client address data structure */

    /* Set the size of the in-out parameter */
    clnt_len = sizeof(echo_clnt_addr);

    /* Wait for a client to connect */
    if ((clnt_sock = accept(serv_sock, (struct sockaddr *) &echo_clnt_addr, &clnt_len)) < 0) {
        dieWithError("accept() failed");
    }

    /* clntSock is connected to a client! */

    printf("Handling client %s %d\n", inet_ntoa(echo_clnt_addr.sin_addr), echo_clnt_addr.sin_port);

    return clnt_sock;
}

void getDecoder(int *decoder, char *filename) {
    int file = open(filename, O_RDONLY, S_IRWXU);
    char letter;
    char line[13];
    int code;
    for (int i = 0; i < 26; ++i) {
        for (int ind = 0; ind < 12; ++ind) {
            int num = read(file, &line[ind], sizeof(char));
            if (num == 0 || line[ind] == '\n' || line[ind] == '\0') {
                line[ind] = '\0';
                break;
            }
        }
        sscanf(line, "%c %d", &letter, &code);
        decoder[letter - 'a'] = code;
    }
}

char getCodedLetter(int *decoder, int code) {
    for (int i = 0; i < 26; ++i) {
        if (decoder[i] == code) {
            return 'a' + i;
        }
    }
    return 0;
}

int readInt(int file, int *p) {
    char line[10];
    for (int ind = 0; ind < 10; ++ind) {
        int num = read(file, &line[ind], sizeof(char));
        if (num == 0 || line[ind] == '\n' || line[ind] == ' ') {
            line[ind] = '\0';
            break;
        }
    }
    if (strlen(line) == 0) {
        return -1;
    }
    sscanf(line, "%d", p);
    return 1;
}
