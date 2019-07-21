//
// Created by ezio on 18/07/19.
//

//function that checks the increasing/decreasing client connections
void checkClientPercentage()
{

    printf("Called percentage checker\n");

    //TODO edit nicenes according to active connections

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
    lastClientNumWhenChecked = *(actual_clients);

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

        printf("Sending SIGUSR1 to handler\n");

        if(kill(handler_info->pid, SIGUSR1) == -1){
            perror("Error in sending SIGUSR1 to handler: ");
            return;
        }
    }

    printf("Percentage checked, old_perc: %f, new_perc: %f\n", old_perc, new_perc);

    //else dont do anything, the number of connected clients is stable
}
