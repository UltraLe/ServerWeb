//
// Created by ezio on 08/08/19.
//

int LOG(int log_type, struct sockaddr_in client)
{
    struct log newLog;
    newLog.client  = client;
    newLog.log_time = clock();
    newLog.log_type = log_type;

    //acquiring sem_on_logs
    if(sem_wait(&sem_on_logs) == -1){
        perror("Error on sem_wait (LOG)");
        return -1;
    }

    //This event should never happen because
    //MAX_LOGS_PER_BRANCH and WRITE_ON_DISK_TIMER_SEC
    //should be set in order to avoid this event
    if(numLogs >= MAX_LOGS_PER_BRANCH){
        printf("A log has been lost\n");

        //releasing sem_on_logs
        if(sem_post(&sem_on_logs) == -1){
            perror("Error on sem_wait (LOG)");
            return -1;
        }

        return -1;
    }

    //inserting log
    branshLogs[numLogs] = newLog;

    //increasing position
    numLogs++;

    //releasing sem_on_logs
    if(sem_post(&sem_on_logs) == -1){
        perror("Error on sem_wait (LOG)");
        return -1;
    }

    return 0;
}

void logger()
{

    while(1){
        sleep(WRITE_ON_DISK_TIMER_SEC);

        //acquiring sem_on_logs
        if(sem_wait(&sem_on_logs) == -1){
            perror("Error on sem_wait (logger)");
            return;
        }

        //coping logs of the server branch
        for(int i = 0; i < numLogs; ++i)
            loggerLogs[i] = branshLogs[i];

        //if there are less logs than MAX_LOGS_PER BRANCH, the logger manager will have to know it
        if(numLogs < MAX_LOGS_PER_BRANCH)
            loggerLogs[numLogs+1].log_type = -1;

        //resetting numLogs
        numLogs = 0;

        //releasing sem_on_logs
        if(sem_post(&sem_on_logs) == -1){
            perror("Error on sem_wait (logger)");
            return;
        }

        //calling the logger manager
        if(sem_post(&(handler_info->sem_awakeLoggerManager)) == -1){
            perror("Error on sem_post (LOG)");
            return;
        }

        //the logger manager could be still sleeping because
        //1. is working on previous logs
        //2. not all the server branches are ready
        if(sem_wait(&(handler_info->sem_loggerManagerHasFinished)) == -1){
            perror("Error on sem_wait (loggermanagerHasFinisched)");
            return;
        }

    }
}