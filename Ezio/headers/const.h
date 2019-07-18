//
// Created by ezio on 16/07/19.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>

#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <unistd.h>

#include <signal.h>

#include <semaphore.h>

#define NUM_INIT_SB 1           //number of the server branches that will be always
                                //ready to serve clients

#define MAX_CLI_PER_SB 512      //number of clients that each server branch will handle

#define CLEANER_CHECK_SEC 120   //number of seconds after which a cleaner will check for
                                //idle client, and close their connection

#define MAX_IDLE_TIME 120       //number of seconds after which will be closed a connection
                                //of an idle client

#define NEW_SB_PERC 0.8         //when the percentage of the (total) connected clients is grater
                                //than this percentage, a new server branch is created

#define MERGE_SB_PERC 0.1       //when the percentage of the (total) connected clients is less
                                //than this percentage, the 2 server branches with less clients will be selected
                                //(i.e. A and B), and the server branch with less connected clients (i.e. A) will send
                                //them to the other branch (i.e. B). This operation will be executed only if
                                //the number of active server branches is grather than NUM_INIT_SB

#define SIGNAL_PERC1 0.1        //every time that a single branch reaches 10%, 50% and 80%
                                //of its maxmimum client capacity, it signal to the handler
#define SIGNAL_PERC2 0.5        //and he will know whether merge or create branches or ignore the signal

#define SIGNAL_PERC3 0.8

#define CACHE_BYTES 1048576     //number of bytes preserved for the cache (10MB)

#define IPC_CACHE_KEY 30        //key to attach to the cache

#define IPC_HNDLR_INFO_KEY 32   //key to attach to the information of the server branches handler

#define IPC_BH_COMM_KEY 34      //key to attach to the memory used to transfer information
                                //between handler and branch

#define MAX_BRANCHES 10000      //used to initialize the memory described up above

#define CHECK_PERC_EACH 10      //check the increasing/decreasing client number (of a server breanch)
                                //every abs(CHECK_PER_EACH) connection recived/closed

#define SERVER_PORT 1033

#define SERVER_ADDR INADDR_ANY

#define BACKLOG 512


char *socket_path = "\0hidden"; //strings that identifies the AF_UNIX socket

struct handler_info{
    pid_t pid;
    int listen_fd;
    sem_t sem_toListenFd;
    sem_t sem_transfClients;
};

struct branch_handler_communication{
    int branch_pid;
    int active_clients;
    sem_t sem_toNumClients;
    char recive_clients;
    char send_clients;
};

struct client_info{
    int fd;
    struct sockaddr_in client_addr;
    time_t last_time_active;
};