//
// Created by ezio on 28/07/19.
//

#include <wait.h>



void sig_chl_handler(int signum)
{
    int status;
    pid_t pid;

    while((pid = waitpid(WAIT_ANY, &status, WNOHANG)) > 0){
        printf("Serer branch %d has terminated\n", pid);
        return;
    }
}



void clients_has_changed()
{
    //the 2 branches with less number of connected clients will be selected
    //and eventually used in merge_branches

    printf("Recived SIGUSR1 in handler, client has changed\n");

    int min = MAX_CLI_PER_SB+1;
    int veryMin = min;
    pid_t minPid = -1, veryMinPid = -1;

    struct branch_handler_communication *minAddr = NULL, *veryMinAddr = NULL;

    int connectedClients = 0;

    int temp;

    //looking for the number of ALL the connected clients
    for(struct branches_info_list *current = first_branch_info; current != NULL; current = current->next){

        if(sem_wait(&((current->info)->sem_toNumClients)) == -1){
            perror("Error in sem_wait (on counting connected clients): ");
            exit(-1);
        }

        temp = (current->info)->active_clients;

        if(sem_post(&((current->info)->sem_toNumClients)) == -1){
            perror("Error in sem_post (on counting connected clients): ");
            exit(-1);
        }

        connectedClients += temp;

        if(temp < veryMin ) {
            min = veryMin;
            minPid = veryMinPid;
            minAddr = veryMinAddr;
            veryMin = temp;
            veryMinPid = (current->info)->branch_pid;
            veryMinAddr = current->info;
        }else if(temp < min){
            min = temp;
            minPid = (current->info)->branch_pid;
            minAddr = current->info;
        }
    }

    //NUM_INIT_SB should never be less than 2, or
    //this logic does not work anymore.

    //if connected clients are grater than the 80% of acceptable clients then
    //create a new branch,
    //if connected clients are less than the 10% of acceptable clients and
    //there are more server branches than NUM_INIT_SB then
    //merge the 2 branches which have the less number of connected clients
    if(connectedClients > NEW_SB_PERC*MAX_CLI_PER_SB*actual_branches_num){

        if(create_new_branch()){
            printf("Error in merge_branches\n");
            exit(-1);
        }

    }else if(connectedClients < MERGE_SB_PERC*MAX_CLI_PER_SB*actual_branches_num
             && actual_branches_num > NUM_INIT_SB){

        //reciver is the minPidClient
        //sender is the veryMinClient
        printf("Merging between sender %d, and reciver %d\n", veryMinPid, minPid);
        if(merge_branches(minPid, minAddr, veryMinPid, veryMinAddr)){
            printf("Error in merge_branches\n");
            exit(-1);
        }
        //printf("Merge ended, actual_branches_num: %d\n", actual_branches_num);

    }

    //branchesStatus();
}
