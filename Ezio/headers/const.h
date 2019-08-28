//
// Created by ezio on 16/07/19.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>

#include <errno.h>

#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <unistd.h>

#include <signal.h>

#include <semaphore.h>

#include <pthread.h>

#include "cacheConst.h"

#define NUM_INIT_SB 4           //number of the server branches that will be always
                                //ready to serve clients

#define MAX_CLI_PER_SB 1024     //number of clients that each server branch will handle (512)

#define CLEANER_CHECK_SEC 90   //number of seconds after which a cleaner will check for
                                //idle client, and close their connection (120)

#define MAX_IDLE_TIME 60        //number of seconds after which will be closed a connection
                                //of an idle client (90)

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

#define CACHE_BYTES sizeof(struct hash_element)*MAX_HASH_KEYS

#define IPC_CACHE_KEY 30        //key to attach to the cache

#define IPC_HNDLR_INFO_KEY 32   //key to attach to the information of the server branches handler

#define IPC_BH_COMM_KEY 34      //key to attach to the memory used to transfer information
                                //between handler and branch

#define MAX_BRANCHES 1000       //used to initialize the memory described up above -> (65536-1026)/MAX_CLI_PER_SB

#define CHECK_PERC_EACH 100       //check the increasing/decreasing client number (of a server breanch)
                                  //every abs(CHECK_PER_EACH) connection recived/closed

#define SERVER_PORT 1033

#define SERVER_ADDR INADDR_ANY

#define BACKLOG 512

#include "loggingConst.h"

char *socket_path = "\0hidden"; //strings that identifies the AF_UNIX socket

struct handler_info{
    pid_t pid;
    int listen_fd;
    sem_t sem_sendRecive;                   //semaphore used to synchronize reciver and sender
    sem_t sem_toListenFd;                   //semaphore used to synchronize server branches to accept clients connection
    sem_t sem_transfClients;                //semaphore used to synchronize handler and reciver and sender
    sem_t sem_awakeLoggerManager;           //semaphore used to synchronize logger and logger manager

    sem_t sem_loggerManagerHasFinished;     //semaphore used by the loggerManager to tell loggers that their
                                            //information has been taken
};

struct branch_handler_communication{
    int branch_pid;
    int active_clients;
    sem_t sem_toNumClients;                 //semaphore to atomically access to the client's number of a server branch
    int recive_clients;
    struct log loggerLogs[MAX_LOGS_PER_BRANCH];
};

struct client_info{
    int fd;
    struct sockaddr_in client_addr;
    time_t last_time_active;
};