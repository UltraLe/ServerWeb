//
// Created by ezio on 18/07/19.
//

#include <sys/un.h>

//function that recive the client connection from a server branch
void recive_clients()
{
    //unix addres type
    struct sockaddr_un addr;
    int unixSock_fd;

    if((unixSock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        perror("Unable to create unix socket: ");
        return;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if(*socket_path == '\0'){
        *addr.sun_path = '\0';
        strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
    }else{
        strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
        unlink(socket_path);
    }

    //connecting to the 'sender of clients'
    if(connect(unixSock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1){
        perror("Error in accept (unix socket): ");
        return;
    }

    char fd_buff[10];
    int fileDescriptorRecived;
    struct sockaddr_in clientAddress;
    ssize_t size_read;

    //reading file descriptors of the sockets
    do{
        memset(fd_buff, 0, sizeof(fd_buff));
        memset(&clientAddress,0, sizeof(clientAddress));

        if((size_read = sock_fd_read(unixSock_fd, fd_buff, sizeof(fd_buff), &fileDescriptorRecived)) < 0){
            perror("Error in sock_fd_read: ");
            exit(-1);
        }else if (size_read == 0){
            //break if the sender closes connection,
            //that means that he has terminated
            break;
        }

        //getting the address of the client recived
        socklen_t len;
        if(getsockname(fileDescriptorRecived, (struct sockaddr *)&clientAddress, &len) == -1){
            perror("Error in getsockname: ");
            exit(-1);
        }

        //memorize the file descriptor recived
        if(insert_new_client(fileDescriptorRecived, clientAddress) == -1){
            printf("Error in insert_new_client (recive_clients)\n");
        }

    }while(1);

    //giving the OK to the handler
    if(sem_post(&(handler_info->sem_transfClients)) == -1){
        perror("Error in sem_post (send_clients): ");
        return;
    }

    //branches dies
    exit(0);
}



//function that sends client connection to an another server branch
//and then the server branch dies
void send_clients()
{
    //unix addres type
    struct sockaddr_un addr;
    int unixSock_fd, unixConnSock;

    if((unixSock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        perror("Unable to create unix socket: ");
        return;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if(*socket_path == '\0'){
        *addr.sun_path = '\0';
        strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
    }else{
        strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
        unlink(socket_path);
    }

    if(bind(unixSock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1){
        perror("Error in bind (unix socket): ");
        return;
    }

    //starting to listen for the reciver
    if(listen(unixSock_fd, 1) == -1){
        perror("Error in listen (unix socket): ");
        return;
    }

    //waiting for the 'reciver of clients'
    if((unixConnSock = accept(unixSock_fd, NULL, NULL)) == -1){
        perror("Error in accept (unix socket): ");
        return;
    }

    char fd_buff[10];
    ssize_t size;

    //sending file descriptors of the sockets
    for(struct client_list *current = firstConnectedClient; current != NULL; current = current->next){

        memset(fd_buff, 0, sizeof(fd_buff));
        sprintf(fd_buff, "%d", (current->client).fd);
        size = sock_fd_write(unixConnSock, fd_buff, strlen(fd_buff), (current->client).fd);

        if(size < 0){
            printf("Error in sock_fd_write\n");
            exit(-1);
        }
    }

    //giving the OK to the handler
    if(sem_post(&(handler_info->sem_transfClients)) == -1){
        perror("Error in sem_post (send_clients): ");
        return;
    }

    //branches dies
    exit(0);

}



//function that close the connection of the inactive clients
void clean()
{
    printf("Clener has just been called\n");

    //getting current time
    time_t now = time(0);

    if(now == (time_t)-1){
        perror("Error in time (clean): ");
        return;
    }

    int time_diff;

    for(struct client_list *current = firstConnectedClient; current != NULL; current = current->next){

        //calculate difference between last time that a client was active and now
        time_diff = (current->client).last_time_active - now;

        //if the client have been inactive for MAX_IDLE_TIME seconds, then colse its connection
        if(time_diff >= MAX_IDLE_TIME)
            if(remove_client(current->client) == -1){
                printf("Error: could not remove the client (clean)\n");
                return;
            }

    }

    //restarting the timer fot the next cleaning operation
    alarm(CLEANER_CHECK_SEC);
}