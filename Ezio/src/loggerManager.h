//
// Created by ezio on 08/08/19.
//

#include <fcntl.h>

int loggerToWait = 0;
struct log *sortedLogs;
char *sortedLogsString;



void releaseWaitingLoggers()
{
    for(int i = 0; i < loggerToWait; ++i){

        if(sem_post(&(info->sem_loggerManagerHasFinished)) == -1){
            perror("Error on sem_post (loggerManagerFinished)");
            exit(-1);
        }

    }
}



int logToString()
{

}



int writeLogsOnDisk()
{
    int fd;
    off_t currentOffset;

    if((fd = open(LOG_FILENAME, O_RDWR)) == -1){
        perror("Error in open (loggerManager)");
        return -1;
    }

    currentOffset = lseek(fd, SEEK_END, 0);

    if(currentOffset == (off_t)-1){
        perror("Error on lseek");
        exit(-1);
    }

    //writing logs
    if(write(fd, sortedLogsString, strlen(sortedLogsString)) == -1){
        perror("Error in writing on file");
        exit(-1);
    }

    close(fd);

}



int sortLoggersLogs()
{
    //the number of logs that has to be sorted
    sortedLogs = (struct log *)malloc(sizeof(struct log)*loggerToWait*MAX_LOGS_PER_BRANCH);
    if(sortedLogs == NULL){
        perror("Error in malloc (sortedLogs\n");
        return -1;
    }

    //TODO sort

    //TODO call logToString


}



void loggerManager()
{

    do{

        if(sem_wait(&semOnBranchesNum) == -1){
            perror("Error on sem_wait (semOnBranchesNum)");
            exit(-1);
        }

        loggerToWait = actual_branches_num;

        if(sem_post(&semOnBranchesNum) == -1){
            perror("Error on sem_post (semOnBranchesNum)");
            exit(-1);
        }

        //logger manager sleeps until 'actual_branches_num' logger are ready
        //to send the log of their clients

        for(int i = 0; i < loggerToWait; ++i){

            if(sem_wait(&(info->sem_awakeLoggerManager)) == -1){
                perror("Error on sem_wait (loggerManager)");
                exit(-1);
            }

        }

        //sorting logs
        if(sortLoggersLogs() == -1){
            printf("Error on sort loggers logs\n");
            exit(-1);
        }

        //writing on disk
        if(writeLogsOnDisk(sortedLogs) == -1){
            printf("Error on sort loggers logs\n");
            exit(-1);
        }

        free(sortedLogs);

        //releasing loggers
        releaseWaitingLoggers();

    }while(1);

}
