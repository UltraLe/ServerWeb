//
// Created by ezio on 18/07/19.
//

#include <sys/resource.h>

//function that checks the increasing/decreasing client connections
void checkClientPercentage()
{

    //printf("Called percentage checker\n");
    float old_perc, new_perc;

    //calculating 'OLD' percentage of connected clients
    old_perc = (float)(lastClientNumWhenChecked*100)/MAX_CLI_PER_SB;

    //printf("lastClientNum: %d, old_perc: %f\n", lastClientNumWhenChecked, old_perc);

    if (0 <= old_perc && old_perc < SIGNAL_PERC1*100) {
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

    if (0 <= new_perc && new_perc < SIGNAL_PERC1*100) {
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

    int priority = 0;

    //every time that OLD PERF is different than NEW PERC,
    //send a signal to the server branches handler
    if(old_perc != new_perc){

        //printf("Sending SIGUSR1 to handler\n");

        if(kill(handler_info->pid, SIGUSR1) == -1){
            perror("Error in sending SIGUSR1 to handler: ");
            return;
        }

        //swhitching priority
        switch((int)new_perc){
            case 1:
                priority = 0;
                break;
            case 2:
                priority = -5;
                break;
            case 3:
                priority = -10;
                break;
            case 4:
                priority = -15;
                break;
            case 5:
                priority = -20;
                break;
        }

        //TODO run with sudo

        if(setpriority(PRIO_PROCESS, getpid(), priority) == -1) {
            perror("Error in setpriority");
        }

    }

    //printf("Percentage checked, old_perc: %f, new_perc: %f\n", old_perc, new_perc);

    //else dont do anything, the number of connected clients is stable
}
