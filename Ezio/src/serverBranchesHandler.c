//
// Created by ezio on 16/07/19.
//

#include "const.h"

int actual_branches_num = 0;

struct handler_info *info;

struct branches_info_list{
    struct branch_handler_communication *info;
    struct branches_info_list *next;
    struct branches_info_list *prev;
};

struct branches_info_list branches_info;
struct branches_info_list *last_branch_info;

struct branch_handler_communication *array_hb;


int look_for_first_array_pos()
{
    for(int i = 0; i < actual_branches_num+1; ++i){

        if((array_hb+i)->branch_pid == -1)
            return i;
    }
    return -1;
}


int create_new_branch()
{
    ///last_branch_info is the pointer to the struct that has to be
    ///filled with the information used by the server branch
    ///which we are going to create right now

    //finding a position of the array_hb...
    int pos = look_for_first_array_pos();

    //...and memorizing it into the information list
    last_branch_info->info = (array_hb + pos);

    //marking the pid in this memory as 'used' by simply giving to him  a value != -1
    (last_branch_info->info)->branch_pid = 0;

    //preparing the 'next' pointer
    if((last_branch_info->next = mmap(NULL, sizeof(struct branches_info_list), PROT_READ|PROT_WRITE,
                                         MAP_ANON|MAP_PRIVATE, 0, 0)) == (void *)-1){
        perror("Unable to mmap (creating new entry in branches_info): ");
        exit(-1);
    }

    //linking the last element of the list
    (last_branch_info->next)->prev = last_branch_info;

    //when the branches handler has to check the global number of client
    //connected, it has to wait for this semaphore for each branch
    //N.B.:last_brench_info->info has to point to some (array_hb + i) where i
    //is the position that has been (already) chosen
    if(sem_init(&((last_branch_info->info)->sem_toNumClients), 1, 1) == -1){
        perror("Error in sem_init (sem_tolistenfd): ");
        exit(-1);
    }

    //giving the position of the array_hb that the branch will own
    char memAddr[5];
    memset(memAddr, 0, sizeof(memAddr));
    sprintf(memAddr, "%d", pos);

    if(!fork()){
        execl("./ServerBranch", "ServerBranch.out", memAddr, NULL);
        perror("Error in execl: ");
        exit(-1);
    }

    actual_branches_num++;

    //setting to the next element of the list
    last_branch_info = last_branch_info->next;

    printf("\t\t\tNew server branch generated\n");

    return 0;
}



int merge_branches(int pid_clientReciver, struct branch_handler_communication *reciver_addr,
                        int pid_clientSender, struct branch_handler_communication *sender_addr)
{
    //the process relative to the 2 branches with less number of clients
    //have to know which one is going to be the reciver and which one
    //is going to be the sender
    //TODO remove send_clients and revice_clients if not needed
    reciver_addr->send_clients = (char)0;
    reciver_addr->recive_clients = (char)1;

    sender_addr->send_clients = (char)1;
    sender_addr->recive_clients = (char)0;

    if(kill(pid_clientReciver, SIGUSR1) == -1){
        perror("Error in kill (SIGUSR1) the 'reciver': ");
        exit(-1);
    }

    if(kill(pid_clientSender, SIGUSR2) == -1){
        perror("Error in kill (SIGUSR2) the 'sender': ");
        exit(-1);
    }

    //waiting that both the reciver and transmitter
    //has finished to transmit connections (file descriptor)
    if(sem_wait(&(info->sem_transfClients)) == -1){
        perror("Error in sem_post (on counting connected clients): ");
        exit(-1);
    }
    if(sem_wait(&(info->sem_transfClients)) == -1){
        perror("Error in sem_post (on counting connected clients): ");
        exit(-1);
    }

    printf("Clients of process '%d' were transmitted to process '%d'\n", pid_clientSender, pid_clientReciver);

    //removing sender branch information from the structure branches_info
    for(struct branches_info_list *current = &branches_info; ; current = current->next){

        if(current->info == sender_addr) {

            (current->info)->branch_pid = -1;

            if (current->prev != NULL) {
                (current->prev)->next = current->next;
            } else {
                //the sender information are situated in first position,
                //the first position has to be moved
                branches_info = *(current->next);
                branches_info.prev = NULL;
            }

            free(current);
            break;
        }

    }

    actual_branches_num--;

    return 0;
}



void clients_has_changed()
{
    //the 2 branches with less number of connected clients will be selected
    //and eventually used in merge_branches
    int min = MAX_CLI_PER_SB+1;
    int veryMin = min;
    pid_t minPid = -1, veryMinPid = -1;

    struct branch_handler_communication *minAddr = NULL, *veryMinAddr = NULL;

    int connectedClients = 0;

    int temp;

    //looking for the number of ALL the connected clients
    for(struct branches_info_list *current = &branches_info; (current->info)->branch_pid != -1; current = current->next){

        if(sem_wait(&((current->info)->sem_toNumClients)) == -1){
            perror("Error in sem_wait (on counting connected clients): ");
            exit(-1);
        }

        temp = (current->info)->active_clients;

        if(sem_post(&((current->info)->sem_toNumClients)) == -1){
            perror("Error in sem_post (on counting connected clients): ");
            exit(-1);
        }

        //TODO test
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

    printf("Number of actual active connections: %d\n", connectedClients);

    if(min + veryMin > MAX_CLI_PER_SB){
        printf("Something went wrong, unexpected values of\nmin: %d, veryMin: %d\n", min, veryMin);
        exit(-1);
    }

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
        if(merge_branches(minPid, minAddr, veryMinPid, veryMinAddr)){
            printf("Error in merge_branches\n");
            exit(-1);
        }

    }
}



int main(int argc, char **argv) {
    int id_info, id_hb;
    pid_t my_pid;
    sem_t sem_tolistenfd, sem_trasfclients;

    int listen_fd;
    struct sockaddr_in address;
    int socket_opt = 1;

    //initializing cache address
    if (shmget(IPC_CACHE_KEY, CACHE_BYTES, IPC_CREAT|0666) == -1) {
        perror("Error in shmget (cache): ");
        exit(-1);
    }

    //initializing address of the struct that will be used by all the server branches
    //to acquire handler creator informations
    if ((id_info = shmget(IPC_HNDLR_INFO_KEY, sizeof(struct handler_info), IPC_CREAT|0666)) == -1) {
        perror("Error in shmget (handler info): ");
        exit(-1);
    }

    //initializing memory that will be used for the transmission of the information
    //between handler and branch
    if ((id_hb = shmget(IPC_BH_COMM_KEY, sizeof(struct branch_handler_communication) * MAX_BRANCHES,
                        IPC_CREAT | 0666)) == -1) {
        perror("Error in shmget (handler-branch): ");
        exit(-1);
    }

    //attaching the memory starting from array_hb. This way we can see this memory
    //as it was an array_hb[MAX_BRANCHES].
    if ((array_hb = shmat(id_hb, array_hb, SHM_R | SHM_W)) == (void *) -1) {
        perror("Error in shmat (array_hb): ");
        exit(-1);
    }

    if ((info = shmat(id_info, NULL, SHM_R | SHM_W)) == (void *) -1) {
        perror("Error in shmat: ");
        exit(-1);
    }

    //initializing semaphore which will be used by all server branches to
    //acquire the listening socket and serve clients
    if (sem_init(&sem_tolistenfd, 1, 1) == -1) {
        perror("Error in sem_init (sem_tolistenfd): ");
        exit(-1);
    }

    //initializing the semaphore that the branches handler will post 2 times
    //after when he decide to merge 2 branches.
    //Each branch involved in this procedure will post this semaphore when
    //the transmission of the clients is completed
    if (sem_init(&sem_trasfclients, 1, 0) == -1) {
        perror("Error in sem_init (sem_trasfclients): ");
        exit(-1);
    }

    //initializing listening socket
    memset(&address, 0, sizeof(address));
    address.sin_addr.s_addr = htonl(SERVER_ADDR);
    address.sin_port = htons(SERVER_PORT);
    address.sin_family = AF_INET;

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error in socket: ");
        exit(-1);
    }

    //TODO check if child (created with execl) becomes zombies when they dies, and handle it

    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &socket_opt, sizeof(socket_opt)) == -1) {
        perror("Error in setsockopt: ");
        exit(-1);
    }

    //binding address
    if(bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) == -1){
        perror("Error in bind: ");
        exit(-1);
    }

    if (listen(listen_fd, BACKLOG) == -1) {
        perror("Error in listen: ");
        exit(-1);
    }

    my_pid = getpid();

    //inserting handler information into the shared structure
    info->pid = my_pid;
    info->listen_fd = listen_fd;
    info->sem_toListenFd = sem_tolistenfd;
    info->sem_transfClients = sem_trasfclients;

    //when the number of the active clients of a branch get to the 10%, 50% and 80%
    //of MAX_CLI_PER_SB, it signals the creator with SIGUSR1.
    //The branches handler will decide whether merge or create branches.
    if (signal(SIGUSR1, clients_has_changed) == SIG_ERR) {
        perror("Error in signal: ");
        exit(-1);
    }

    //initializing the list of branches
    last_branch_info = &branches_info;

    //initializing the array. Operation needed to distinguish
    //free positions
    printf("Initializing shared memory\n");
    for (int i = 0; i < MAX_BRANCHES; ++i){
        (array_hb +i)->branch_pid = -1;
    }
    printf("Shared memory initialized\n");

    //generating the initial server branches
    for(int i = 0; i < NUM_INIT_SB; ++i) {
        if (create_new_branch()) {
            printf("Error in create_new_branch (generating point)\n");
            exit(-1);
        }
    }

    //waiting for signals to come
    pause();

    printf("\t\t\tWARNING !!!\nServer branches handler has just returned\n");
}