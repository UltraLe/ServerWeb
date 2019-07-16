//
// Created by ezio on 16/07/19.
//

#include "const.h"

int create_new_branch()
{

}

int merge_branches()
{

}

void clients_has_changed(int  signum)
{

}

int actual_branches_num = 0;

int main(int argc, char **argv)
{
    int id_info, id_cache;
    pid_t my_pid;
    sem_t sem_tolistenfd, sem_trasfclients;
    struct handler_info *info;

    int listen_fd;
    struct sockaddr_in address;
    int socket_opt = 1;

    struct branch_handler_communication *branches_info;

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
    info->sem_tolistenfd = &sem_tolistenfd;
    info->sem_transfclients = &sem_trasfclients;

    //when the number of the active clients of each branch get to the 10%, 50% and 80%
    //of MAX_CLI_PER_SB, it signals the creator with SIGUSR1.
    //The branches handler will decide whether merge or create branches.
    if(signal(SIGUSR1, clients_has_changed) == SIG_ERR){
        perror("Error in signal: ");
        exit(-1);
    }

    //initializing the memory populated with branch_handler_communication structures
    //that the handler creator will use to send information to a specific server branch
    if((branches_info = mmap(NULL, MAX_NUM_SB*sizeof(struct branch_handler_communication),
                        PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, 0, 0)) == (void *)-1){
        perror("Error in mmap: ");
        exit(-1);
    }




}


