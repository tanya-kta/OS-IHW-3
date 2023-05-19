#include "../info.h"
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>


void handleObserver(int serv_sock) {
    int clnt_socket = acceptTcpConnection(serv_sock);

    sem_post(&obs_p->parent_sem);
    while (1) {
        sem_wait(&obs_p->child_sem);
        if (obs_p->type == MSG_TYPE_FINISH) {
            break;
        }
        int size = strlen(obs_p->message);
        if (send(clnt_socket, &obs_p->message, size, 0) != size) {
            dieWithError("send() failed");
        }
        sem_post(&obs_p->parent_sem);
    }
    close(shmid);
    close(clnt_socket);
    close(serv_sock);
    sem_post(&obs_p->parent_sem);
}


int main(int argc, char *argv[]) {
    int serv_sock;                     /* Socket descriptor for server */
    unsigned short echo_serv_port;     /* Server port */

    if (argc != 5) {
        fprintf(stderr, "Usage:  %s <Clients number> <Input File Path> <Output File Path> <Server Port>\n", argv[0]);
        exit(1);
    }

    echo_serv_port = atoi(argv[4]);

    serv_sock = createTcpServerSocket(echo_serv_port);
    process_number = atoi(argv[1]);

    if ((shmid = shm_open(mem_name, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
        perror("shm_open");
        dieWithError("server: object is already open");
    } else {
        printf("Object is open: name = %s, id = 0x%x\n", mem_name, shmid);
    }
    // Задание размера объекта памяти
    if (ftruncate(shmid, sizeof(message_t) * process_number + sizeof(observer_t)) == -1) {
        perror("ftruncate");
        dieWithError("server: memory sizing error");
    } else {
        printf("Memory size set and = %lu\n", sizeof(message_t) * process_number + sizeof(observer_t));
    }
    // получить доступ к памяти
    void *pointer = mmap(0, sizeof(message_t) * process_number + sizeof(observer_t),
                         PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0);
    msg_p = pointer;
    obs_p = pointer + sizeof(message_t) * process_number;

    for (int i = 0; i < process_number; ++i) {
        if (sem_init(&msg_p[i].child_sem, 1, 0) == -1) {
            dieWithError("Creating child semaphore went wrong");
        }
        if (sem_init(&msg_p[i].parent_sem, 1, 0) == -1) {
            dieWithError("Creating parent semaphore went wrong");
        }
    }
    if (sem_init(&obs_p->child_sem, 1, 0) == -1) {
        dieWithError("Creating child semaphore went wrong");
    }
    if (sem_init(&obs_p->parent_sem, 1, 0) == -1) {
        dieWithError("Creating parent semaphore went wrong");
    }
    prev = signal(SIGINT, parentHandleCtrlC);

    for (int i = 0; i < process_number; ++i) {
        pid_t process_id;
        if ((process_id = fork()) < 0) {
            dieWithError("fork() failed");
        } else if (process_id == 0) {
            signal(SIGINT, prev);
            handleTcpClient(serv_sock, i);
            exit(0);
        }
    }

    for (int i = 0; i < process_number; ++i) { // дождаться, когда все клиенты подключатся к процессам сервера
        sem_wait(&msg_p[i].parent_sem);
    }

    pid_t obs_id;
    if ((obs_id = fork()) < 0) {
        dieWithError("fork() failed");
    } else if (obs_id == 0) {
        signal(SIGINT, prev);
        handleObserver(serv_sock);
        exit(0);
    }
    sem_wait(&obs_p->parent_sem);
    obs_p->type = MSG_TYPE_STRING;

    char message[10000];
    int in_file = open(argv[2], O_RDONLY, S_IRWXU);
    int out_file = open(argv[3], O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
    int status = 1;
    while (status == 1) {
        int num_of_running = 0;
        for (int i = 0; i < process_number; ++i, ++num_of_running) {
            int size = 0;
            sprintf(message, "Send to client id %d message to decode: ", i);
            int last_char = strlen(message);
            for (; size < MAX_INTS; ++size) {
                status = readInt(in_file, &msg_p[i].coded[size]);
                if (status == -1) {
                    break;
                }
                sprintf(message + last_char, "%d ", msg_p[i].coded[size]);
                last_char = strlen(message);
            }
            if (size == 0) {
                break;
            }
            msg_p[i].size = size;
            msg_p[i].type = MSG_TYPE_INT;
            sprintf(obs_p->message, "%s", message);
            sem_post(&msg_p[i].child_sem);
            sem_post(&obs_p->child_sem);
            sem_wait(&obs_p->parent_sem);
        }

        for (int i = 0; i < num_of_running; ++i) {
            sem_wait(&msg_p[i].parent_sem);
            sprintf(message, "Received from client id %d decoded message: ", i);
            int last_char = strlen(message);
            printf("server parent from child id %d process after client decoding: ", i);
            for (int j = 0; j < msg_p[i].size; ++j) {
                printf("%c", msg_p[i].uncoded[j]);
                sprintf(message + last_char, "%c", msg_p[i].uncoded[j]);
                last_char = strlen(message);
                write(out_file, &msg_p[i].uncoded[j], 1);
            }
            printf("\n");
            sprintf(obs_p->message, "%s", message);
            sem_post(&obs_p->child_sem);
            sem_wait(&obs_p->parent_sem);
        }
    }
    close(in_file);
    close(out_file);

    for (int i = 0; i < process_number; ++i) {
        msg_p[i].type = MSG_TYPE_FINISH;
        sem_post(&msg_p[i].child_sem);
    }
    obs_p->type = MSG_TYPE_FINISH;
    sem_post(&obs_p->child_sem);
    for (int i = 0; i < process_number; ++i) {
        sem_wait(&msg_p[i].parent_sem);
    }
    sem_wait(&obs_p->parent_sem);
    for (int i = 0; i < process_number; ++i) {
        sem_destroy(&msg_p[i].child_sem);
        sem_destroy(&msg_p[i].parent_sem);
    }
    sem_destroy(&obs_p->parent_sem);
    sem_destroy(&obs_p->child_sem);
    close(shmid);
    if (shm_unlink(mem_name) == -1) {
        perror("shm_unlink");
        dieWithError("server: error getting pointer to shared memory");
    }
    process_number++;
    while (process_number) /* Clean up all zombies */
    {
        int process_id = waitpid((pid_t) -1, NULL, WNOHANG);  /* Non-blocking wait */
        if (process_id < 0)  /* waitpid() error? */ {
            dieWithError("waitpid() failed");
        } else if (process_id == 0)  /* No zombie to wait on */ {
            break;
        } else {
            process_number--; /* Cleaned up after a child */
        }
    }
    close(serv_sock);    /* Close client socket */
}
