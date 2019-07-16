//
// Created by ezio on 16/07/19.
//

#include "const.h"

int actual_branches_num = 0;

struct branches_info_list{
    struct branch_handler_communication *info;
    struct branches_info_list *next;
    struct branches_info_list *prev;
};

struct branches_info_list branches_info;
struct branches_info_list *last_branch_info;

int create_new_branch()
{
    //initializing the memory populated with branch_handler_communication structures
    //that the handler creator will use to send information to a specific server branch
    if((last_branch_info->info = mmap(NULL, sizeof(struct branch_handler_communication),
                                        PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, 0, 0)) == (void *)-1){
        perror("Unable to mmap (inserting new branch_handler_communication): ");
        exit(-1);
    }

    if(((last_branch_info->info)->sem_toNumClients = mmap(NULL, sizeof(sem_t),
                                      PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, 0, 0)) == (void *)-1){
        perror("Unable to mmap (inserting new branch_handler_communication): ");
        exit(-1);
    }

    //when the branches handler has to check the global number of client
    //connected, it has to post this semaphore for each branch
    if(sem_init((last_branch_info->info)->sem_toNumClients, 1, 1) == -1){
        perror("Error in sem_init (sem_tolistenfd): ");
        exit(-1);
    }

    char memAddr[20];
    memset(memAddr, 0, sizeof(memAddr));
    sprintf(memAddr, "%p", last_branch_info->info);

    if(!fork()){
        execl("./ServerBranch.out", "ServerBranch.out", memAddr, NULL);
        perror("Error in execl: ");
        exit(-1);
    }else{
        wait();
    }

    actual_branches_num++;

    return 0;
}

int merge_branches(int pid_clientReciver, int pid_clientSender)
{
    //TODO merge operation
}

void clients_has_changed(int  signum)
{
    //the 2 branches with less number of connected clients will be selected
    //and eventually used in merge_branches
    int min = MAX_CLI_PER_SB+1;
    int veryMin = min;
    pid_t minPid = -1, veryMinPid = -1;

    int connectedClients = 0;

    int temp;

    //looking for the number of ALL the connected clients
    for(struct branches_info_list *current = &branches_info; current != NULL; current = current->next){
        if(sem_wait((current->info)->sem_toNumClients) == -1){
            perror("Error in sem_post (on counting connected clients): ");
            exit(-1);
        }

        temp = (current->info)->active_clients;

        if(sem_post((current->info)->sem_toNumClients) == -1){
            perror("Error in sem_post (on counting connected clients): ");
            exit(-1);
        }

        //TODO test
        connectedClients += temp;
        if(temp < veryMin ) {
            min = veryMin;
            minPid = veryMinPid;
            veryMin = temp;
            veryMinPid = (current->info)->branch_pid;
        }else if(temp < min){
            min = temp;
            minPid = (current->info)->branch_pid;
        }
    }

    if(min + veryMin > MAX_CLI_PER_SB){
        printf("Something went wrong, unexpected values of\nmin: %d, veryMin: %d\n", min, veryMin);
        exit(-1);
    }

    //if connected clients are grather than the 80% of acceptable clients then
    //create a new branch
    //if connected clients are less than the 10% of acceptable clients then
    //merge the 2 branches which have the less number of connected clients
    if(connectedClients > NEW_SB_PERC*MAX_CLI_PER_SB*actual_branches_num){

        if(!create_new_branch()){
            printf("Error in merge_branches\n");
            exit(-1);
        }

    }else if(connectedClients < MERGE_SB_PERC*MAX_CLI_PER_SB*actual_branches_num){

        //reciver is the minPidClient
        //sender is the veryMinClient
        if(!merge_branches(minPid, veryMinPid)){
            printf("Error in merge_branches\n");
            exit(-1);
        }

    }
}

int main(int argc, char **argv)
{
    int id_info, id_cache;
    pid_t my_pid;
    sem_t sem_tolistenfd, sem_trasfclients;
    struct handler_info *info;

    int listen_fd;
    struct sockaddr_in address;
    int socket_opt = 1;

    //initializing cache address
    if((id_cache =  shmget(IPC_CACHE_KEY, CACHE_BYTES, IPC_CREAT|IPC_EXCL|0666)) == -1){
        perror("Error in shmget (cache): ");
        exit(-1);
    }

    //initializing address of the struct that will be used by all the server branches
    //to acquire handler creator informations
    if((id_info =  shmget(IPC_HNDLR_INFO_KEY, sizeof(struct handler_info), IPC_CREAT|IPC_EXCL|0666)) == -1){
        perror("Error in shmget (handler info): ");
        exit(-1);
    }

    if((info = shmat(id_info, NULL, SHM_R|SHM_W)) == (void *)-1){
        perror("Error in shmat: ");
        exit(-1);
    }

    //initializing semaphore which will be used by all server branches to
    //acquire the listening socket and serve clients
    if(sem_init(&sem_tolistenfd, 1, 1) == -1){
        perror("Error in sem_init (sem_tolistenfd): ");
        exit(-1);
    }

    //initializing the semaphore that the branches handler will post 2 times
    //(setting its value to zero), after when he decide to merge 2 branches.
    //Each branch involved in this procedure will post this semaphore when
    //the transmisison of the clients is completed
    if(sem_init(&sem_trasfclients, 1, 2) == -1){
        perror("Error in sem_init (sem_trasfclients): ");
        exit(-1);
    }

    //initializing listening socket
    memset(&address, 0, sizeof(address));
    address.sin_addr.s_addr = htonl(SERVER_ADDR);
    address.sin_port = htons(SERVER_PORT);
    address.sin_family = AF_INET;

    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("Error in socket: ");
        exit(-1);
    }

    if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &socket_opt, sizeof(socket_opt)) == -1){
        perror("Error in setsockopt: ");
        exit(-1);
    }

    if(listen(listen_fd, BACKLOG) == -1){
        perror("Error in listen: ");
        exit(-1);
    }

    my_pid = getpid();

    //inserting handler information into the shared structure
    info->pid = my_pid;
    info->listen_fd = listen_fd;
    info->sem_toListenFd = &sem_tolistenfd;
    info->sem_transfClients = &sem_trasfclients;

    //when the number of the active clients of a branch get to the 10%, 50% and 80%
    //of MAX_CLI_PER_SB, it signals the creator with SIGUSR1.
    //The branches handler will decide whether merge or create branches.
    if(signal(SIGUSR1, clients_has_changed) == SIG_ERR){
        perror("Error in signal: ");
        exit(-1);
    }

    last_branch_info = &branches_info;

    //generating the initial server branches
    for(int i = 0; i < NUM_INIT_SB; ++i) {
        if (!create_new_branch()) {
            printf("Error in create_new_branch (generating point)\n");
            exit(-1);
        }
    }

    //waiting for signals to come
    pause();

    printf("\t\t\tWARNING !!!\nServer branches handler has just returned\n");
}