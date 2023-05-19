#include "../info.h"
#include <sys/wait.h>       /* for waitpid() */


void handleTcpClient(int clnt_socket)
{
    char echoBuffer[MAX_INTS];        /* Buffer for echo string */
    int recvMsgSize;                    /* Size of received message */

    /* Receive message from client */
    if ((recvMsgSize = recv(clnt_socket, echoBuffer, MAX_INTS, 0)) < 0)
        dieWithError("recv() failed");

    /* Send received string and receive again until end of transmission */
    while (recvMsgSize > 0)      /* zero indicates end of transmission */
    {
        /* Echo message back to client */
        if (send(clnt_socket, echoBuffer, recvMsgSize, 0) != recvMsgSize)
            dieWithError("send() failed");

        /* See if there is more data to receive */
        if ((recvMsgSize = recv(clnt_socket, echoBuffer, MAX_INTS, 0)) < 0)
            dieWithError("recv() failed");
    }

    close(clnt_socket);    /* Close client socket */
}

int main(int argc, char *argv[])
{
    int serv_sock;                     /* Socket descriptor for server */
    int clnt_sock;                     /* Socket descriptor for client */
    unsigned short echo_serv_port;     /* Server port */
    pid_t process_id;                  /* Process ID from fork() */
    unsigned int child_proc_count = 0; /* Number of child processes */

    if (argc != 4)     /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage:  %s <Input File Path> <Output File Path> <Server Port>\n", argv[0]);
        exit(1);
    }

    echo_serv_port = atoi(argv[3]);  /* First arg:  local port */

    serv_sock = createTcpServerSocket(echo_serv_port);

    clnt_sock = acceptTcpConnection(serv_sock);
    int in_file = open(argv[1], O_RDONLY, S_IRWXU);
    int out_file = open(argv[2], O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
    int status = 1;
    int buffer[MAX_INTS];
    char decoded[MAX_INTS];
    while (status == 1) {
        int size = 0;
        for (; size < MAX_INTS; ++size) {
            status = readInt(in_file, &buffer[size]);
            if (status == -1) {
                break;
            }
        }
        if (size == 0) {
            break;
        }
        if (send(clnt_sock, buffer, size * 4, 0) != size * 4) {
            dieWithError("send() failed");
        }
        printf("parent from child id \n");
        int recv_msg_size;
        if ((recv_msg_size = recv(clnt_sock, decoded, MAX_INTS, 0)) < 0) {
            dieWithError("recv() failed");
        }
        for (int j = 0; j < recv_msg_size; ++j) {
            printf("%c", decoded[j]);
            write(out_file, &decoded[j], 1);
        }
        printf("\n");
    }
    close(in_file);
    close(out_file);

}
