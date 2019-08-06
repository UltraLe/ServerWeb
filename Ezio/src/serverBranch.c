//
// Created by ezio on 16/07/19.
//

#include "const.h"
#include "fileDescriptorTransmission.h"

#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <asm/errno.h>
#include <pthread.h>
#include "writen.h"

#define READ_BUFFER_BYTE 4096

struct client_list{
    struct client_info client;
    struct client_list *next;
    struct client_list *prev;
};

struct handler_info *handler_info;
struct client_list *firstConnectedClient;
struct client_list *lastConnectedClient;

sem_t *sem_cli;
int *shouldRecive;
int *actual_clients;
int lastClientNumWhenChecked = 0;

short serverIsFull = 0;

//TODO variable used for detect loop bug, remove when is solved
//int lastFdServed = 0, strike;

fd_set readSet, allSet, writeSet;
int numSetsReady = 0, max_fd;

sem_t remaningToServe;
sem_t allServed;
sem_t clientToServe;
//each client has to be removed atomically
sem_t removing;

#include "checkClientPercentage.h"

//functions that insert a new client into the connected client list
int insert_new_client(int connect_fd, struct sockaddr_in clientAddress)
{
    //printf("\tServer branch with pid %d inserting client...\n", getpid());


    struct client_list *new_entry;

    if((*actual_clients) == MAX_CLI_PER_SB)
        return -1;

    //inserting client into the last position (which has to be initialized)
    if(sem_wait(sem_cli) == -1){
        perror("Unable to sem_wait (insert_new_client): ");
        return -1;
    }
    *actual_clients += 1;
    if(sem_post(sem_cli) == -1){
        perror("Unable to sem_post (insert_new_client): ");
        return -1;
    }

    //inserting client information into the client list (last element)
    if((new_entry = (struct client_list *)malloc(sizeof(struct client_list))) == NULL){
        perror("Error in malloc (insert_new_client): ");
        return -1;
    }
    memset(&(new_entry->client), 0, sizeof(struct client_info));

    (new_entry->client).fd = connect_fd;
    (new_entry->client).client_addr = clientAddress;
    (new_entry->client).last_time_active = time(0);

    //initializing the semaphore used by the writer threads
    if (sem_init(&((new_entry->client).serving), 1, 1) == -1) {
        perror("Error in sem_init (serving): ");
        exit(-1);
    }

    //inserting client into the set
    FD_SET(connect_fd, &allSet);

    //TODO qui aggiornamento del file log ?

    if(connect_fd > max_fd) {
        max_fd = connect_fd;
    }

    //linking last element with previous

    //if there are no client in the server branch
    if(firstConnectedClient == NULL){
        firstConnectedClient = new_entry;
        firstConnectedClient->prev = NULL;
        firstConnectedClient->next = NULL;
        lastConnectedClient = firstConnectedClient;
    }else{
        //if there are other clients
        (lastConnectedClient->next) = new_entry;
        new_entry->prev = lastConnectedClient;

        lastConnectedClient = lastConnectedClient->next;
        lastConnectedClient->next = NULL;
    }

    if(abs(*(actual_clients)-lastClientNumWhenChecked) >= CHECK_PERC_EACH)
        checkClientPercentage();

    return 0;
}



//function that close (and remove) a client connection
int remove_client(struct client_info client)
{
    //printf("\tServer branch with pid %d into remove_client\n", getpid());
    
    //acquiring semaphore to remove
    if(sem_wait(&removing) == -1){
        perror("Error in sem_wait (removing)");
        exit(-1);
    }

    for(struct client_list *current = firstConnectedClient; current != NULL; current = current->next){

        //removing client from connectedClient list
        if((current->client).fd == client.fd){

            if(close(client.fd) == -1){
                perror("Unable to close connection (remove_client): ");
                exit(-1);
            }

            //remove client from allSet
            //printf("Removing %d\n", client.fd);
            FD_CLR(client.fd, &allSet);


            //handling the list of clients

            //if the element is the head of the list then
            //make the next element the new head list
            if((current->prev) == NULL){
                firstConnectedClient = current->next;

                if(firstConnectedClient != NULL)
                    firstConnectedClient->prev = NULL;

            }else if((current->next) == NULL){
                lastConnectedClient = (current->prev);
                (lastConnectedClient->next) = NULL;
            }else{
                ((current->prev)->next) = (current->next);
                (current->next)->prev = current->prev;
            }

            //free current
            free(current);

            if(sem_wait(sem_cli) == -1){
                perror("Unable to sem_wait (insert_new_client): ");
                return -1;
            }
            *actual_clients -= 1;
            if(sem_post(sem_cli) == -1){
                perror("Unable to sem_post (insert_new_client): ");
                return -1;
            }

            //making the server branch able to accept new connection
            //if it is no more full.
            //if server is full and i am removing a client
            //then it is no more full
            if(serverIsFull) {
                FD_SET(handler_info->listen_fd, &allSet);
                serverIsFull = 0;
            }

            if(abs(*(actual_clients)-lastClientNumWhenChecked) >= CHECK_PERC_EACH)
                checkClientPercentage();

            //TODO qui aggiornamento file log

            //printf("\tServer branch with pid %d client removed\n", getpid());

            //posting semaphore to remove
            if(sem_post(&removing) == -1){
                perror("Error in sem_wait (removing)");
                exit(-1);
            }

            return 0;
        }
    }

    //printf("\tServer branch with pid %d fd to remove not found\n", getpid());

    return -1;
}



#include "branchEventsHandlers.h"
#include "writerThread.h"



void clientStatus(int pos)
{
    printf("\nChild %d, pid %d\n", pos, getpid());
    for(struct client_list *current = firstConnectedClient; current != NULL; current = current->next) {
        printf("Client fd: %d\n", (current->client).fd);
    }
}



int main(int argc, char **argv)
{
    if(argc < 2){
        printf("Server branches cannot access to array position\n");
        exit(-1);
    }

    int position = atoi(argv[1]);
    pid_t pid = getpid();
    struct branch_handler_communication *talkToHandler;

    //attaching to the branch_handler_communication struct, in order to communicate
    //to the server branch handler
    int id_hb;

    if((id_hb = shmget(IPC_BH_COMM_KEY, sizeof(struct branch_handler_communication), 0)) == -1){
        perror("Error in shmget (getting branch_handler_communication structure): ");
        exit(-1);
    }
    if((talkToHandler = shmat(id_hb, NULL, SHM_R|SHM_W)) == (void *)-1){
        perror("Error in shmat (attahcing to branch_andler_com struct): ");
        exit(-1);
    }

    //going to the correct position of the ''array''
    talkToHandler += position;

    //printf("Given position from (talkToHandler): %p to (talkToHandler+1):%p\n", talkToHandler, talkToHandler+1);

    //initializing data which will be used to talk with the handler
    actual_clients = &(talkToHandler->active_clients);
    *actual_clients = 0;
    talkToHandler->branch_pid = pid;
    sem_cli = &(talkToHandler->sem_toNumClients);
    shouldRecive = &(talkToHandler->recive_clients);


    //attaching to handler 'global' information structure
    int id_handlerInfo;
    if((id_handlerInfo = shmget(IPC_HNDLR_INFO_KEY, sizeof(struct handler_info), 0666)) == -1){
        perror("Error in shmget (getting handler_info structure): ");
        exit(-1);
    }
    if((handler_info = shmat(id_handlerInfo, NULL, SHM_R|SHM_W)) == (void *)-1){
        perror("Error in shmat (attaching to handler_info struct): ");
        exit(-1);
    }

    //attaching to cache
    void *cache;
    int id_cache;
    if((id_cache = shmget(IPC_HNDLR_INFO_KEY, sizeof(struct handler_info), 0666)) == -1){
        perror("Error in shmget (getting handler_info structure): ");
        exit(-1);
    }
    if((cache = shmat(id_handlerInfo, NULL, SHM_R|SHM_W)) == (void *)-1){
        perror("Error in shmat (attaching to handler_info struct): ");
        exit(-1);
    }

    //the server branch will react to SIGUSR2 by
    //setting up an AF_UNIX socket through with it
    //will send the connections to another server branch
    //and return when the operation is complete

    struct sigaction act2;
    memset(&act2, 0, sizeof(act2));
    act2.sa_handler = send_or_recive;
    act2.sa_flags = 0;

    if(sigaction(SIGUSR2, &act2, NULL) == -1){
        perror("Error in sigaction (SIGUSR2): ");
        exit(-1);
    }

    //setting up the cleaner
    if(signal(SIGALRM, clean) == SIG_ERR){
        perror("Error in sigaction (clean): ");
        exit(-1);
    }
    //starting its timer
    alarm(CLEANER_CHECK_SEC);


    ///here starts the main body of the server branch
    int connect_fd, tryWaitRet;

    //initializing semaphores used by writer threads
    if (sem_init(&remaningToServe, 1, 1) == -1) {
        perror("Error in sem_init (remaningToServe): ");
        exit(-1);
    }

    if (sem_init(&clientToServe, 1, 0) == -1) {
        perror("Error in sem_init (clientToServe): ");
        exit(-1);
    }

    if (sem_init(&allServed, 1, 0) == -1) {
        perror("Error in sem_init (allServed): ");
        exit(-1);
    }
    
    //semaphore uset to stnchronize writers while removing clients
    if (sem_init(&removing, 1, 1) == -1) {
        perror("Error in sem_init (remaningToServe): ");
        exit(-1);
    }

    pthread_t pidt;

    for(int i = 0; i < NUM_WRITERS; ++i){
        if(pthread_create(&pidt, NULL, (void *)handleRequest, NULL)){
            perror("Error in create writers");
            exit(-1);
        }
    }

    struct sockaddr_in acceptedClientAddress;
    socklen_t lenCliAddr;

    //initializing client list
    firstConnectedClient = NULL;
    lastConnectedClient = firstConnectedClient;

    struct timeval time;
    memset(&time, 0, sizeof(time));
    time.tv_usec = 0;
    time.tv_sec = 0;
    int thereIsClient;
    fd_set checkListenSet, listenSet;

    //initializing the set
    FD_ZERO(&allSet);

    FD_SET(handler_info->listen_fd, &allSet);

    listenSet = allSet;

    max_fd = handler_info->listen_fd;

    printf("\tServer branch with pid %d ready\n", getpid());
    //ready to serve clients
    while(1){

        //resetting errno
        errno = 0;

        //resetting readSet
        readSet = allSet;

        //printf("\tServer branch with pid %d waiting on the select\n", getpid());

        if((numSetsReady = (select(max_fd + 1, &readSet, NULL, NULL, NULL))) == -1){

            //may be interrubted by a signal
            if(errno == EINTR)
                continue;

            perror("Error in select: ");
            exit(-1);
        }

        //TRYING to establish a connection with a client
        while(FD_ISSET(handler_info->listen_fd, &readSet)) {

            //printf("\tServer branch wih pid: %d in trywait\n", getpid());

            //acquiring semaphore on the listening socket
            tryWaitRet = sem_trywait(&(handler_info->sem_toListenFd));

            if (errno == EAGAIN) {
                //handle other client requests (if there are any) if sempahore was not acquired

                //printf("\tServer branch wih pid: %d did not acquired semaphore\n", getpid());
                numSetsReady--;
                break;

            } else if (errno == EINTR) {
                //ignore the client to be accepted, and check if other clients has to be handled

                printf("The sem_trywait has been interrupted by a signal\n");
                numSetsReady--;
                break;

            } else if (tryWaitRet == -1) {
                perror("Error in sem_trywait");
                exit(-1);
            }

            //printf("\tServer branch wih pid: %d acquired semaphore\n", getpid());

            memset(&acceptedClientAddress, 0, sizeof(acceptedClientAddress));

            //Lets suppose that a server branch (A) returns from a select
            //because a client has to be accepted. Lets say that another server branch (B) acquires the
            //semaphore, accept the client and post the semaphore. In this case
            //the server branch (A) could get stuck in accept system call because
            //the client has been already accepted (by B).
            //To avoid this situation a second select (with a timer of 0 seconds -> polling) function is needed to be sure
            //that another client is ready to be accepted.
            while(1) {
                checkListenSet = listenSet;

                thereIsClient = select((handler_info->listen_fd) + 1, &checkListenSet, NULL, NULL, &time);
                //printf("Polling DONE, thereIsClient: %d\n", thereIsClient);

                //posting the semaphore, if there are no client to accept
                if(thereIsClient == 0) {
                    if (sem_post(&(handler_info->sem_toListenFd)) == -1) {
                        perror("Error in sem_post (sem_toListenFd): ");
                        exit(-1);
                    }

                    break;
                }

                if(thereIsClient == -1 && errno == EINTR){
                    //if the select is interrupted by a signal, repeat it
                    printf("Select with timer has been interrupted\n");
                    //resetting errno before repeating the loop
                    errno = 0;
                    continue;

                }else if(thereIsClient == -1 && errno != EINTR){
                    perror("Error in select with the timer");
                    exit(-1);
                }

                break;
            }

            while (thereIsClient){

                //printf("There is a client\n");

                //if the branch is here he has to take care of the connection :)
                connect_fd = accept(handler_info->listen_fd, (struct sockaddr *) &acceptedClientAddress, &lenCliAddr);

                //if the syscall has been interrupted by a signal, then restart it
                if (connect_fd == -1 && errno == EINTR) {
                    //resetting errno before repeating the loop
                    errno = 0;
                    continue;
                }

                //if the server branch is here another error occurred
                if(connect_fd == -1 && errno != EINTR){
                    perror("Error in accept");
                    exit(-1);
                }
                //posting the semaphore, once accepted the connection
                if (sem_post(&(handler_info->sem_toListenFd)) == -1) {
                    perror("Error in sem_post (sem_toListenFd): ");
                    exit(-1);
                }

                //printf("\tServer branch wih pid: %d has posted semaphore\n", getpid());

                //TODO loop may be caused by this, to fix later
                if (insert_new_client(connect_fd, acceptedClientAddress) == -1) {
                    printf("Cannot accept client, max capacity has been reached\n");
                }

                //if the server branch
                //reaches the max capacity, ignore the next connections
                if ((*actual_clients) == MAX_CLI_PER_SB) {
                    FD_CLR(handler_info->listen_fd, &allSet);
                    serverIsFull = 1;
                }

               //printf("\tServer branch with pid %d has inserted clients\n", getpid());

                break;
            }

            //the server branch is here if
            //1. he has accepted the connection of a client
            //2. he tried to accept it but another server branch has already accepted it
            numSetsReady--;
            break;
        }

        //look for other descriptors
        if (numSetsReady <= 0)
            continue;

        //right here numSetsReady contain the clients that need to be served
        //increasing remaining to serve

        //coping the sets to be served into writeSet
        writeSet = readSet;
        int numToWait = numSetsReady;
        //posting 'numSetsReady' times, allowing 'numSetsReady' writers to 'activate'
        for(int i = 0; i < numToWait; ++i) {
            if (sem_post(&clientToServe) == -1) {
                perror("Error in sem_post (clientToServe): ");
                exit(-1);
            }
        }

        //waiting the writers to complete their job, if the server branch
        //would not wait them, the writeSet would be overwritten, and
        //the requests of clients would be lost
        printf("Actual numSetsReady: %d\n", numSetsReady);

        for(int i = 0; i < numToWait; ++i){
            if (sem_wait(&allServed) == -1) {
                perror("Error in sem_post (allServed): ");
                exit(-1);
            }
        }

        //here numSetsReady should be 0
        if(numSetsReady > 0)
            printf("Something went erong, numsetsReady: %d\n", numSetsReady);

    }
}