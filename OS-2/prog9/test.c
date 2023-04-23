#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#define MSGSZ 128

typedef struct message {
    long mtype;
    char mtext[MSGSZ];
} message;

typedef struct data {
    int id;
    char name[MSGSZ];
} data;

int main(void) {
    key_t key = ftok(".", 'm');
    int msqid;

    if ((msqid = msgget(key, 0666 | IPC_CREAT)) == -1) {
        perror("msgget");
        exit(1);
    }

    printf("Message queue created with ID %d\n", msqid);

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(1);
    } else if (pid == 0) { // child process
        message msg_rcv, msg_snd;
        data d;

        if (msgrcv(msqid, &msg_rcv, sizeof(message), 1, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }

        printf("Child process received message: %s\n", msg_rcv.mtext);

        memcpy(&d, msg_rcv.mtext, sizeof(data)); // convert message text to data struct

        printf("Child process received data: id=%d name=%s\n", d.id, d.name);

        // modify data struct
        d.id++;
        strcat(d.name, " Jr.");

        // convert data struct back to message text
        memcpy(msg_snd.mtext, &d, sizeof(data));
        msg_snd.mtype = 2;

        if (msgsnd(msqid, &msg_snd, sizeof(message), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }

        printf("Child process sent modified data: id=%d name=%s\n", d.id, d.name);

        exit(0);
    } else { // parent process
        message msg_rcv, msg_snd;
        data d;

        d.id = 1234;
        strcpy(d.name, "John Doe");

        printf("Parent process created data: id=%d name=%s\n", d.id, d.name);

        // convert data struct to message text
        memcpy(msg_snd.mtext, &d, sizeof(data));
        msg_snd.mtype = 1;

        if (msgsnd(msqid, &msg_snd, sizeof(message), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }

        printf("Parent process sent data to child process\n");

        if (msgrcv(msqid, &msg_rcv, sizeof(message), 2, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }

        printf("Parent process received message: %s\n", msg_rcv.mtext);

        memcpy(&d, msg_rcv.mtext, sizeof(data)); // convert message text to data struct

        printf("Parent process received modified data: id=%d name=%s\n", d.id, d.name);

        exit(0);
    }
}
