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

    //inserting log
    logs[numLogs] = newLog;

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
    struct log tempLogs[MAX_LOGS_PER_BRANCH];

    while(1){
        sleep(WRITE_ON_DISK_TIMER_SEC);

        //acquiring sem_on_logs
        if(sem_wait(&sem_on_logs) == -1){
            perror("Error on sem_wait (LOG)");
            return;
        }

        //coping logs of the server branch
        for(int i = 0; i < numLogs; ++i)
            tempLogs[i] = logs[i];

        //TODO implementare come il manager capisce che ha finito i log di una singola branch

        if(numLogs < MAX_LOGS_PER_BRANCH)
            tempLogs[numLogs+1].log_type = -1;

        //resetting numLogs
        numLogs = 0;

        //releasing sem_on_logs
        if(sem_post(&sem_on_logs) == -1){
            perror("Error on sem_wait (LOG)");
            return;
        }

        //calling the logger manager


    }
}