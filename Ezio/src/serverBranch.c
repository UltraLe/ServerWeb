//
// Created by ezio on 16/07/19.
//

#include "const.h"
#include "writen.h"

#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <asm/errno.h>
#include <errno.h>


#define READ_BUFFER_BYTE 4096


struct client_list{
    struct client_info client;
    struct client_list *next;
    struct client_list *prev;
};

struct client_list connectedClients;
struct client_list *lastConnectedClient;

int actual_clients = 0;
int lastClientNumWhenChecked = 0;

int numSetsReady, max_fd;
fd_set readSet, allSet;


void checkClientPercentage()
{
    lastClientNumWhenChecked = actual_clients;

}


int insert_new_client(int connect_fd, struct sockaddr_in *clientAddress)
{
    //inserting client into the last position (which has to be initialized)
    if((actual_clients++) == MAX_CLI_PER_SB)
        return -1;

    //inserting client information into the client list
    (lastConnectedClient->client).fd = connect_fd;
    (lastConnectedClient->client).client_addr = *clientAddress;
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

    if(abs(actual_clients-lastClientNumWhenChecked) >= CHECK_PERC_EACH)
            checkClientPercentage();

    return 0;
}



int remove_client(struct client_info client)
{
    for(struct client_list *current = &connectedClients; (current->client).fd != -1; current = current->next){

        //removing client from connectedClient list
        if((current->client).fd == client.fd){

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

            free(&current);

            if(abs(actual_clients-lastClientNumWhenChecked) >= CHECK_PERC_EACH)
                checkClientPercentage();

            //TODO qui aggiornamento file log

            return 0;
        }
    }

    return -1;
}



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
            remove_client(client);
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
    }

    return 0;
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
    int *activeClients = &(talkToHandler->active_clients);
    talkToHandler->branch_pid = pid;
    sem_t *sem_cli = &(talkToHandler->sem_toNumClients);


    //attaching to handler 'global' information structure
    struct handler_info *handler_info;
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

    //TODO implement after server features
    //redirecting signals SIGUSR1, SIGUSR2
    // ...

    //TODO implemet cleaner thread creation, that checks inactive connections



    ///here starts the main body of the server branch
    int connect_fd;

    struct sockaddr_in acceptedClientAddress;
    socklen_t lenCliAddr;
    memset(&acceptedClientAddress, 0, sizeof(acceptedClientAddress));

    //initializing client list
    lastConnectedClient = &connectedClients;
    (lastConnectedClient->client).fd = -1;

    //initializing the set
    FD_ZERO(&allSet);
    FD_SET(handler_info->listen_fd, &allSet);

    max_fd = handler_info->listen_fd;

    //ready to serve clients
    while(1){

        //resetting readSet
        readSet = allSet;

        if((numSetsReady = (select(max_fd +1, &readSet, NULL, NULL, NULL))) == -1){
            perror("Error in select: ");
            exit(-1);
        }

        printf("Select returned something\n");

        //if a new client try to connect to the web server,
        //and the server branch can handle other connection
        if(FD_ISSET(handler_info->listen_fd, &readSet) && actual_clients != MAX_CLI_PER_SB) {

            //try to accept its connection
            sem_trywait(&(handler_info->sem_toListenFd));

            switch (errno) {
                case EAGAIN:
                    //do nothing, the semaphore has been already acquired
                    break;
                case -1:
                    perror("Error in sem_trywait:");
                    exit(-1);
            }

            //if the branch is here he has to take care of the connection :)
            if ((connect_fd = accept(handler_info->listen_fd,
                                     (struct sockaddr *) &acceptedClientAddress, &lenCliAddr)) == -1) {
                perror("Error in accept: ");
                exit(-1);
            }

            printf("Clientaccepted\n");

            //posting the semaphore, once accepted the connection
            if (sem_post(&(handler_info->sem_toListenFd)) == -1) {
                perror("Error in sem_post (sem_toListenFd): ");
                exit(-1);
            }

            if (insert_new_client(connect_fd, &acceptedClientAddress) == -1) {
                printf("Cannot accept client, max capacity has been reached\n");
            }

            printf("Client inserted\n");

            numSetsReady--;

            printf("numSetsReady: %d\n", numSetsReady);

            //look for other descriptors
            if (numSetsReady <= 0)
                continue;

        }

        for(struct client_list *current = &connectedClients; (current->client).fd != -1 && numSetsReady > 0;
                                                                                current = current->next){

            handleRequest(current->client);
            numSetsReady--;
        }

        //request handled

    }
}