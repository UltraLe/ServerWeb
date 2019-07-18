//
// Created by ezio on 18/07/19.
//

//function that recive the client connection from a server branch
void recive_clients()
{

}



//function that sends client connection to an another server branch
//and then the server branch dies
void send_clients()
{

}



//function that close the connection of the inactive clients
void clean()
{
    //getting current time
    time_t now = time(0);

    if(now == (time_t)-1){
        perror("Error in time (clean): ");
        return;
    }

    int time_diff;

    for(struct client_list *current = &connectedClients; (current->client).fd != -1; current = current->next){

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


//function that checks the increasing/decreasing client connections
void checkClientPercentage()
{
    float old_perc, new_perc;

    //calculating 'OLD' percentage of connected clients
    old_perc = (float)(lastClientNumWhenChecked*100)/MAX_CLI_PER_SB;

    if (0 <= old_perc < SIGNAL_PERC1*100) {
        old_perc = 1;
    }else if(SIGNAL_PERC1*100 <= old_perc && old_perc < SIGNAL_PERC2*100) {
        old_perc = 2;
    }else if(SIGNAL_PERC2*100 <= old_perc && old_perc < SIGNAL_PERC3*100) {
        old_perc = 3;
    }else if(SIGNAL_PERC3*100 <= old_perc && old_perc < 100) {
        old_perc = 4;
    }else if(old_perc == 100) {
        old_perc = 5;
    }else {
        printf("Something went wrong (OLD perc)\n");
        return;
    }

    //memorizing the number of connecten clients when this function is called
    lastClientNumWhenChecked = actual_clients;

    //calculating 'NEW' (actual) percentage of connected clients
    new_perc = (float)(lastClientNumWhenChecked*100)/MAX_CLI_PER_SB;

    if (0 <= new_perc < SIGNAL_PERC1*100) {
        new_perc = 1;
    }else if(SIGNAL_PERC1*100 <= new_perc && new_perc < SIGNAL_PERC2*100) {
        new_perc = 2;
    }else if(SIGNAL_PERC2*100 <= new_perc && new_perc < SIGNAL_PERC3*100) {
        new_perc = 3;
    }else if(SIGNAL_PERC3*100 <= new_perc && new_perc < 100) {
        new_perc = 4;
    }else if(new_perc == 100) {
        new_perc = 5;
    }else {
        printf("Something went wrong (NEW perc)\n");
        return;
    }

    //every time that OLD PERF is different than NEW PERC,
    //send a signal to the server branches handler
    if(old_perc != new_perc){
        if(kill(handler_info->pid, SIGUSR1) == -1){
            perror("Error in sending SIGUSR1 to handler: ");
            return;
        }
    }

    //else dont do anything, the number of connected clients is stable

}