//
// Created by ezio on 16/07/19.
//

#include "const.h"
#include "fileDescriptorTransmission.h"

#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <asm/errno.h>
#include <errno.h>
#include "writen.h"

#define READ_BUFFER_BYTE 4096

struct client_list{
    struct client_info client;
    struct client_list *next;
    struct client_list *prev;
};

struct handler_info *handler_info;
struct client_list connectedClients;
struct client_list *lastConnectedClient;

sem_t *sem_cli;
int *actual_clients;
int lastClientNumWhenChecked = 0;

int numSetsReady, max_fd;
fd_set readSet, allSet;

#include "checkClientPercentage.h"

//functions that insert a new client into the connected client list
int insert_new_client(int connect_fd, struct sockaddr_in clientAddress)
{
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

    //inserting client information into the client list
    (lastConnectedClient->client).fd = connect_fd;
    (lastConnectedClient->client).client_addr = clientAddress;
    (lastConnectedClient->client).last_time_active = time(0);

    //inserting client into the set
    FD_SET(connect_fd, &allSet);

    //TODO qui aggiornamento del file log ?

    if(connect_fd > max_fd)
        max_fd = connect_fd;

    //initializing next 'last' client element list
    lastConnectedClient->next = (struct client_list*)malloc(sizeof(struct client_list));
    (lastConnectedClient->next)->prev = lastConnectedClient;

    lastConnectedClient = lastConnectedClient->next;
    (lastConnectedClient->client).fd = -1;

    if(abs(*(actual_clients)-lastClientNumWhenChecked) >= CHECK_PERC_EACH)
        checkClientPercentage();

    return 0;
}



//function that close (and remove) a client connection
int remove_client(struct client_info client)
{
    for(struct client_list *current = &connectedClients; (current->client).fd != -1; current = current->next){

        //removing client from connectedClient list
        if((current->client).fd == client.fd){

            if(close(client.fd) == -1){
                perror("Unable to close connection (remove_client): ");
                exit(-1);
            }

            //remove client from allSet
            FD_CLR(client.fd, &allSet);

            (current->client).fd = -1;
            //if the element is the head of the list then
            //make the next element the new head list
            if(current->prev == NULL){
                connectedClients = *(current->next);
                connectedClients.prev = NULL;
            }else{
                (current->prev)->next = current->next;
            }

            //free only if it is not the first element
            if(current != &connectedClients)
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

            if(abs(*(actual_clients)-lastClientNumWhenChecked) >= CHECK_PERC_EACH)
                checkClientPercentage();

            //TODO qui aggiornamento file log

            return 0;
        }
    }

    return -1;
}



#include "eventsHandlers.h"



//function that handles a client HTTP request
int handleRequest(struct client_info client)
{
    int numByteRead;
    char readBuffer[READ_BUFFER_BYTE];

    char *writeBuffer;

    //USATO PER TEST RIMUOVERE
    char *http_header = "HTTP/1.1 200 OK\nVary: Accept-Encoding\nContent-Type: text/html\nAccept-Ranges: bytes\nLast-Modified: Mon, 17 Jul 2017 19:28:15 GMT\nContent-Length: 103\nDate: Sun, 14 Jul 2019 10:44:37 GMT\nServer: lighttpd/1.4.35";
    char *http_body = "\n\n<html><head><title>Benvenuto</title></head><body><div align=”center”>Hello World!</div></body></html>\n\n";
    char http[4096];
    strcat(http, http_header);
    strcat(http, http_body);
    //FINE ROBA DA TOGLIERE DOPO IL MERGE

    if(FD_ISSET(client.fd, &readSet)){


        if((numByteRead = read(client.fd, readBuffer, sizeof(readBuffer))) == 0){
            //client has closed connection
            if(remove_client(client) == -1){
                printf("Error: could not remove the client (handleRequest)\n");
                return -1;
            }
        }else{

            //USATA PER TEST
            if(writen(client.fd, http, strlen(http)) < 0){
                perror("Error in writen (unable to reply): ");
                return -1;
            }
            //FINE ROBA DA TOGLIERE DOPO IL MERGE

            //TODO qui funzioni che gestiscono
            //TODO richiesta HTTP
            //TODO elaborazione coefficiente di adattamento
            //TODO accesso in cache per (eventualmente) prelevare l'immagine
            //TODO invio della risposta HTTP
            //TODO inserimento (eventuale) dell'immagine adattata in cache
        }

        numSetsReady--;
    }

    return 0;
}



void clientStatus(int pos)
{
    printf("\nChild %d\n", pos);
    for(struct client_list *current = &connectedClients; (current->client).fd != -1; current = current->next)
        printf("Client fd: %d\n", (current->client).fd);
}



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
        perror("Error in shmget (getting branch_handler_communication structure): ");
        exit(-1);
    }
    if((talkToHandler = shmat(id_hb, NULL, SHM_R|SHM_W)) == (void *)-1){
        perror("Error in shmat (attahcing to branch_andler_com struct): ");
        exit(-1);
    }

    //going to the correct position of the ''array''
    talkToHandler += position;

    //initializing data which will be used to talk with the handler
    actual_clients = &(talkToHandler->active_clients);
    *actual_clients = 0;
    talkToHandler->branch_pid = pid;
    sem_cli = &(talkToHandler->sem_toNumClients);


    //attaching to handler 'global' information structure
    int id_handlerInfo;
    if((id_handlerInfo = shmget(IPC_HNDLR_INFO_KEY, sizeof(struct handler_info), 0)) == -1){
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
    if((id_cache = shmget(IPC_HNDLR_INFO_KEY, sizeof(struct handler_info), 0)) == -1){
        perror("Error in shmget (getting handler_info structure): ");
        exit(-1);
    }
    if((cache = shmat(id_handlerInfo, NULL, SHM_R|SHM_W)) == (void *)-1){
        perror("Error in shmat (attaching to handler_info struct): ");
        exit(-1);
    }

    //the server branch will react to SIGUSR1 by
    //setting up an AF_UNIX socket through with it
    //will recive the connections of another server branch
    if(signal(SIGUSR1, recive_clients) == SIG_ERR){
        perror("Error in signal (SIGUSR1): ");
        exit(-1);
    }

    //the server branch will react to SIGUSR2 by
    //setting up an AF_UNIX socket through with it
    //will send the connections to another server branch
    //and return when the operation is complete
    if(signal(SIGUSR2, send_clients) == SIG_ERR){
        perror("Error in signal (SIGUSR2): ");
        exit(-1);
    }


    //setting up the cleaner
    if(signal(SIGALRM, clean) == SIG_ERR){
        perror("Error in signal (clean): ");
        exit(-1);
    }
    //starting its timer
    alarm(CLEANER_CHECK_SEC);


    ///here starts the main body of the server branch
    int connect_fd, tryWaitRet;

    struct sockaddr_in acceptedClientAddress;
    socklen_t lenCliAddr;

    //initializing client list
    lastConnectedClient = &connectedClients;
    (lastConnectedClient->client).fd = -1;

    //initializing the set
    FD_ZERO(&allSet);
    FD_SET(handler_info->listen_fd, &allSet);

    max_fd = handler_info->listen_fd;

    //ready to serve clients
    while(1){

        //resetting errno
        errno = 0;

        //resetting readSet
        readSet = allSet;

        //clientStatus(position);
        if((numSetsReady = (select(max_fd +1, &readSet, NULL, NULL, NULL))) == -1){
            perror("Error in select: ");
            exit(-1);
        }

        //if a new client try to connect to the web server,
        //and the server branch can handle other connection
        if(FD_ISSET(handler_info->listen_fd, &readSet) && (*actual_clients) != MAX_CLI_PER_SB) {

            //try to accept its connection
            tryWaitRet = sem_trywait(&(handler_info->sem_toListenFd));

            if(errno == EAGAIN){
                continue;
            }else if(tryWaitRet == -1){
                perror("Error in sem_trywait:");
                exit(-1);
            }

            printf("Child %d acquired semaphore\n", position);

            memset(&acceptedClientAddress, 0, sizeof(acceptedClientAddress));

            //if the branch is here he has to take care of the connection :)
            if ((connect_fd = accept(handler_info->listen_fd,
                                     (struct sockaddr *) &acceptedClientAddress, &lenCliAddr)) == -1) {
                perror("Error in accept: ");
                exit(-1);
            }

            if (insert_new_client(connect_fd, acceptedClientAddress) == -1) {
                printf("Cannot accept client, max capacity has been reached\n");
            }

            numSetsReady--;

            //posting the semaphore, once accepted the connection
            if (sem_post(&(handler_info->sem_toListenFd)) == -1) {
                perror("Error in sem_post (sem_toListenFd): ");
                exit(-1);
            }

            //look for other descriptors
            if (numSetsReady <= 0)
                continue;

        }

        for(struct client_list *current = &connectedClients; (current->client).fd != -1 && numSetsReady > 0;
                                                                                current = current->next){
            if(handleRequest(current->client) == -1){
                printf("Error: could not handleRequest (main)\n");
                continue;
            }
        }
        clientStatus(position);
        //request handled
    }
}