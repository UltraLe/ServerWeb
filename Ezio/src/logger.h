//
// Created by ezio on 08/08/19.
//

int LOG(int log_type, struct sockaddr_in client)
{
    struct log newLog;
    newLog.client  = client;
    newLog.log_time = time(0);
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
    branchLogs[numLogs] = newLog;

    //increasing position
    numLogs++;

    //releasing sem_on_logs
    if(sem_post(&sem_on_logs) == -1){
        perror("Error on sem_wait (LOG)");
        return -1;
    }

    //printf("Logger with pid %d has created a log (%d)\n", getpid(), log_type);

    return 0;
}

void logger()
{

    while(1){
        //printf("Logger with pid %d is sleeping\n", getpid());

        sleep(WRITE_ON_DISK_TIMER_SEC);

        //acquiring sem_on_logs
        if(sem_wait(&sem_on_logs) == -1){
            perror("Error on sem_wait (logger)");
            return;
        }

        //coping logs of the server branch
        for(int i = 0; i < numLogs; ++i)
            loggerLogs[i] = branchLogs[i];

        //if there are less logs than MAX_LOGS_PER BRANCH, the logger manager will have to know it
        if(numLogs < MAX_LOGS_PER_BRANCH)
            (loggerLogs[numLogs]).log_type = -1;

        //printf("Branch %d, numLogs (total registered): %d\n", getpid(), numLogs);

        //resetting numLogs and branchLogs
        numLogs = 0;
        memset(branchLogs, 0, sizeof(struct log)*MAX_LOGS_PER_BRANCH);

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
        //printf("Logger with pid %d is waiting for the manager\n", getpid());
        if(sem_wait(&(handler_info->sem_loggerManagerHasFinished)) == -1){
            perror("Error on sem_wait (loggermanagerHasFinisched)");
            return;
        }
        //printf("Logger with pid %d has finished\n", getpid());
    }
}