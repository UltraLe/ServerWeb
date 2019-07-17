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
    pid_t pid = getpid();

    //attaching to the branch_handler_communication struct, in order to communicate
    //to the server branch handler
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

    //going to the correct position of the ''array''
    talkToHandler += position;

    //initializing data to talk to the handler
    int *activeClients = &(talkToHandler->active_clients);
    talkToHandler->branch_pid = pid;
    sem_t *sem_cli = &(talkToHandler->sem_toNumClients);


    pause();
}