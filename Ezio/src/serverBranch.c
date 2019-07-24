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
struct client_list *firstConnectedClient;
struct client_list *lastConnectedClient;

sem_t *sem_cli;
int *actual_clients;
int lastClientNumWhenChecked = 0;

short serverIsFull = 0;

int numSetsReady, max_fd;
fd_set readSet, allSet;

#include "checkClientPercentage.h"

//functions that insert a new client into the connected client list
int insert_new_client(int connect_fd, struct sockaddr_in clientAddress)
{
    printf("\tServer branch with pid %d inserting client...\n", getpid());


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

    //inserting client into the set
    FD_SET(connect_fd, &allSet);

    //TODO qui aggiornamento del file log ?

    if(connect_fd > max_fd)
        max_fd = connect_fd;

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
    printf("\tServer branch with pid %d into remove_client\n", getpid());

    for(struct client_list *current = firstConnectedClient; current != NULL; current = current->next){

        //removing client from connectedClient list
        if((current->client).fd == client.fd){

            if(close(client.fd) == -1){
                perror("Unable to close connection (remove_client): ");
                exit(-1);
            }

            //remove client from allSet
            printf("Removing %d\n", client.fd);
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

            printf("\tServer branch with pid %d client removed\n", getpid());

            return 0;
        }
    }

    printf("\tServer branch with pid %d fd to remove not found\n", getpid());

    return -1;
}



#include "eventsHandlers.h"



//function that handles a client HTTP request
int handleRequest(struct client_info *client)
{
    int numByteRead;
    char readBuffer[READ_BUFFER_BYTE];
    memset(readBuffer, 0, sizeof(readBuffer));

    char *writeBuffer;

    //USATO PER TEST RIMUOVERE
    char *http_header = "HTTP/1.1 200 OK\nVary: Accept-Encoding\nContent-Type: text/html\nAccept-Ranges: bytes\nLast-Modified: Mon, 17 Jul 2017 19:28:15 GMT\nContent-Length: 180\nDate: Sun, 14 Jul 2019 10:44:37 GMT\nServer: lighttpd/1.4.35";
    char *http_body = "\n\n<!DOCTYPE html>\n"
                      "<html>\n"
                      "<body>\n"
                      "<h1>Hello World !</h1>\n"
                      "<p></p>\n"
                      "<h1>Ciao Mondo !</h1>\n"
                      "<p>(per Giovanni)</p>\n"
                      "<h1>Bella Fra !</h1>\n"
                      "<p>(per Riccardo)</p>\n"
                      "<h1>UuuuoooOOoooo !</h1>\n"
                      "<p>(per Ezio)</p>\n"
                      "</body>\n"
                      "</html>";

    char http[4096];
    strcat(http, http_header);
    strcat(http, http_body);
    //FINE ROBA DA TOGLIERE DOPO IL MERGE

    if(FD_ISSET(client->fd, &readSet)){

        if((numByteRead = read(client->fd, readBuffer, sizeof(readBuffer))) == 0){
            //client has closed connection
            if(remove_client(*client) == -1){
                printf("Error: could not remove the client (handleRequest)\n");
                return -1;
            }
        }else if(numByteRead == -1) {
            perror("Error in read client information: ");

            //TODO handle resetting connection requested by peers

            remove_client(*client);

            return -1;
        }else{

            //Updating client's last time active
            client->last_time_active = time(0);

            //USATA PER TEST
            if(writen(client->fd, http, strlen(http)) < 0){
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

        printf("\tServer branch with pid %d has finisched to handle the request of fd = %d\n", getpid(), client->fd);
    }

    return 0;
}



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

    printf("Given position from (talkToHandler): %p to (talkToHandler+1):%p\n", talkToHandler, talkToHandler+1);

    //initializing data which will be used to talk with the handler
    actual_clients = &(talkToHandler->active_clients);
    *actual_clients = 0;
    talkToHandler->branch_pid = pid;
    sem_cli = &(talkToHandler->sem_toNumClients);


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

    //the server branch will react to SIGUSR1 by
    //setting up an AF_UNIX socket through with it
    //will recive the connections of another server branch
    if(signal(SIGUSR1, (void *)recive_clients) == SIG_ERR){
        perror("Error in signal (SIGUSR1): ");
        exit(-1);
    }

    //the server branch will react to SIGUSR2 by
    //setting up an AF_UNIX socket through with it
    //will send the connections to another server branch
    //and return when the operation is complete
    if(signal(SIGUSR2, (void *)send_clients) == SIG_ERR){
        perror("Error in signal (SIGUSR2): ");
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

    struct sockaddr_in acceptedClientAddress;
    socklen_t lenCliAddr;

    //initializing client list
    firstConnectedClient = NULL;
    lastConnectedClient = firstConnectedClient;

    //initializing the set
    FD_ZERO(&allSet);
    FD_SET(handler_info->listen_fd, &allSet);

    max_fd = handler_info->listen_fd;

    printf("\tServer branch with pid %d ready\n", getpid());
    //ready to serve clients
    while(1){

        //resetting errno
        errno = 0;

        //resetting readSet
        readSet = allSet;

        //clientStatus(position);
        printf("\tServer branch with pid %d waiting on the select\n", getpid());
        if((numSetsReady = (select(max_fd +1, &readSet, NULL, NULL, NULL))) == -1){

            //may be interrubted by a signal
            if(errno == EINTR)
                continue;

            perror("Error in select: ");
            exit(-1);
        }

        //if a new client try to connect to the web server,
        //and the server branch can handle other connection
        if(FD_ISSET(handler_info->listen_fd, &readSet)) {

            //try to accept its connection
            tryWaitRet = sem_trywait(&(handler_info->sem_toListenFd));

            if(errno == EAGAIN){
                continue;
            }else if(tryWaitRet == -1){
                perror("Error in sem_trywait:");
                exit(-1);
            }

            memset(&acceptedClientAddress, 0, sizeof(acceptedClientAddress));

            //if the branch is here he has to take care of the connection :)
            if ((connect_fd = accept(handler_info->listen_fd,
                                     (struct sockaddr *) &acceptedClientAddress, &lenCliAddr)) == -1) {
                perror("Error in accept: ");
                exit(-1);
            }

            //posting the semaphore, once accepted the connection
            if (sem_post(&(handler_info->sem_toListenFd)) == -1) {
                perror("Error in sem_post (sem_toListenFd): ");
                exit(-1);
            }

            if (insert_new_client(connect_fd, acceptedClientAddress) == -1) {
                printf("Cannot accept client, max capacity has been reached\n");
            }

            //if the server branch
            //reaches the max capacity, ignore the next connections
            if((*actual_clients) == MAX_CLI_PER_SB){
                FD_CLR(handler_info->listen_fd, &allSet);
                serverIsFull = 1;
            }

            numSetsReady--;

            printf("\tServer branch with pid %d has inserted clients\n", getpid());

        }

        //look for other descriptors
        if (numSetsReady <= 0)
            continue;

        printf("\tServer branch with pid %d has numSetsReady = %d, starting handling clients\n", getpid(), numSetsReady);

        for(struct client_list *current = firstConnectedClient; current != NULL && numSetsReady > 0; current = current->next){

            //TODO WHY THIS PREVENTS TO STCK SMASCHING
            printf("fd: %d, address: %p\n", (current->client).fd, &(current->client));

            if(handleRequest(&(current->client)) == -1){
                printf("Error: could not handleRequest (main)\n");
                //numSetsReady--;
                continue;
            }

        }

        printf("\tServer branch with pid %d has finisched to handle the clients (numSetsReady = %d)\n", getpid(), numSetsReady);

        //TODO WHY THIS PREVENT TO LOOP THE CLIENTS REQUEST
        clientStatus(position);
    }
}

/*
 * Server branch a volte no risponde più ai client                              (RISOLTO)
 * Server branch solo quando è piena a volte risponde in loop un client         (RISOLTO, messo in todo)
 * Segnale di ricezione dei client non arriva a destinazione                    (RISOLVERE prima quelli sopra)
 * Stack smasching                                                              (se non ci sta la printf nel for)
 */