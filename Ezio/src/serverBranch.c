//
// Created by ezio on 16/07/19.
//

#include "const.h"

int main(int argc, char **argv)
{
    if(argc < 2){
        printf("Server branches cannot access to array position\n");
        exit(-1);
    }

    int position = atoi(argv[1]);

    printf("position passed to child: %d\n", position);

    struct branch_handler_communication *talkToHandler;

    int id_hb;

    if((id_hb = shmget(IPC_BH_COMM_KEY, sizeof(struct branch_handler_communication), 0)) == -1){
        perror("Error in shmget (shared memory does not exist): ");
        exit(-1);
    }

    if((talkToHandler = shmat(id_hb, NULL, SHM_R|SHM_W)) == (void *)-1){
        perror("Error in shmat (branch): ");
        exit(-1);
    }

    talkToHandler += position;

    printf("PID VALUE: %d\n", talkToHandler->branch_pid);
    talkToHandler->branch_pid = getpid();
    printf("pid updated\n");

    if(sem_post(&(talkToHandler->sem_toNumClients)) == -1){
        perror("Error in sem_post (on counting connected clients): ");
        exit(-1);
    }


    pause();
}