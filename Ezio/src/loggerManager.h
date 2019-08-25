//
// Created by ezio on 08/08/19.
//

#include <fcntl.h>
#include <time.h>

int loggerToWait = 0;
char *sortedLogsString;
int sortedLogsStringSize;

int timeLenTemplate;
int cliLenTemplate;



void releaseWaitingLoggers()
{
    //printf("Releasing loggers\n");

    for(int i = 0; i < loggerToWait; ++i){

        if(sem_post(&(info->sem_loggerManagerHasFinished)) == -1){
            perror("Error on sem_post (loggerManagerFinished)");
            exit(-1);
        }

    }
}



int logToString(struct log currentLog)
{
    char stringMessage[MAX_LOG_LEN];
    char stringClient[cliLenTemplate];
    char stringTime[timeLenTemplate];

    memset(stringMessage, 0, MAX_LOG_LEN);
    memset(stringClient, 0, cliLenTemplate);
    memset(stringTime, 0, timeLenTemplate);


    //printf("Called logToSting\n");


    char stringLog[MAX_LOG_LEN+cliLenTemplate+timeLenTemplate];
    memset(stringLog, 0, MAX_LOG_LEN+cliLenTemplate+timeLenTemplate);

    //client action in string0
    //printf("0, log_type: %d\n", currentLog.log_type);

    switch(currentLog.log_type){

        case CLIENT_ACCEPTED:
            strcpy(stringMessage, CLIENT_ACCEPTED_S);
            //printf("1\n");
            break;
        case CLIENT_IDLE:
            strcpy(stringMessage, CLIENT_IDLE_S);
            //printf("2\n");
            break;
        case CLIENT_ERROR_READ:
            strcpy(stringMessage, CLIENT_ERROR_READ_S);
            //printf("3\n");
            break;
        case CLIENT_REMOVED:
            strcpy(stringMessage, CLIENT_REMOVED_S);
            //printf("4\n");
            break;
        case CLIENT_SERVED:
            strcpy(stringMessage, CLIENT_SERVED_S);
            //printf("5\n");
            break;
        case CLIENT_MERGED:
            strcpy(stringMessage, CLIENT_MERGED_S);
            //printf("6\n");
            break;
        default:
            printf("Something went wrong in logToString\n");
            return -1;
    }

    //log time in string
    struct tm *localTime = localtime(&(currentLog.log_time));
    sprintf(stringTime, "[%s", asctime(localTime));

    strcpy((stringTime + timeLenTemplate - 3), "]->");

    //client info in string
    sprintf(stringClient, "[%s:%i]->", inet_ntoa(currentLog.client.sin_addr), ntohs(currentLog.client.sin_port));

    //creating log string
    sprintf(stringLog, "%s%s%s", stringTime, stringClient, stringMessage);

    //printf("stringLog: %s\n", stringLog);

    //linking to the sorted ones
    strcat(sortedLogsString, stringLog);

    //printf("After strcat\n");

    return 0;
}



int writeLogsOnDisk()
{
    int fd;
    off_t currentOffset;

    if((fd = open(LOG_FILENAME, O_RDWR|O_APPEND|O_CREAT, 0666)) == -1){
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

    return 0;

}



int sortLoggersLogs()
{
    //vector of idexes
    int *branchIndex;

    branchIndex = (int *)malloc(sizeof(int)*loggerToWait);
    if(branchIndex == NULL){
        perror("Error in malloc (branchIndex\n");
        return -1;
    }

    //initializing indexes
    for(int i = 0; i < loggerToWait; ++i)
        branchIndex[i] = 0;

    //vector used to check if a branch has finished its logs
    int *branchCompleted;

    branchCompleted = (int *)malloc(sizeof(int)*loggerToWait);
    if(branchCompleted == NULL){
        perror("Error in malloc (branchCompleted\n");
        return -1;
    }

    //initializing branchCompleted
    for(int i = 0; i < loggerToWait; ++i)
        branchCompleted[i] = 0;

    //allocating space for the sorted stringLogs
    sortedLogsString = (char *)malloc(sizeof(char)*loggerToWait*MAX_LOGS_PER_BRANCH*(MAX_LOG_LEN+cliLenTemplate+timeLenTemplate));
    time_t constMin = time(NULL) + 1;
    time_t min = constMin;
    time_t tempLogClock;

    int j;
    int minIndex;

    struct log *tempLog;
    struct log minLog;
    int minSelected;

    for (int i = 0; i < loggerToWait; ) {

        j = 0;
        minIndex = 0;
        minSelected = 0;

        for (struct branches_info_list *current = first_branch_info; j < loggerToWait; current = current->next, ++j) {

            //if a branch has been completed, then jump to the next one
            if(branchCompleted[j] == 1)
                continue;

            //printf("Branch %d\n", (current->info)->branch_pid);

            //printf("Index: %d\n", branchIndex[j]);

            tempLog = (((current->info)->loggerLogs) + branchIndex[j]);

            //if there are no more logs for the current branch,
            //increase i, and continue
            if((tempLog->log_type) == -1){
                ++i;
                //printf("This branch has finished\n");
                branchCompleted[j] = 1;
                continue;
            }

            tempLogClock = tempLog->log_time;
            //printf("tempClock: %ld, minClock: %ld\n", tempLogClock, min);

            if (tempLogClock < min) {
                minSelected = 1;
                min = tempLogClock;
                minIndex = j;
                minLog = *tempLog;
                //printf("A new min time has been registered, with clock: %ld\n", tempLog->log_time);
            }

        }

        if(minSelected) {

            if (branchIndex[minIndex] == MAX_LOGS_PER_BRANCH && branchCompleted[j] == 0) {
                ++i;
                branchCompleted[j] = 1;
            }

            //increasing index of branch that has the minimum (in clock) log
            branchIndex[minIndex]++;

            //here the minimum has been selected, converted and appended
            //printf("Before calling logToString\n");

            if (logToString(minLog) == -1) {
                printf("Error in logToString, branch: numToWait: %d\n", loggerToWait);
                return -1;
            }

            //resetting min
            min = constMin;

            //printf("After calling logToString\n");
        }
    }

    //printf("All: %s\n", sortedLogsString);

    //printf("Finished sorting\n");

    return 0;
}



void loggerManager()
{
    //initializing data used by the loggerManager
    timeLenTemplate = strlen("[Sat Aug 10 15:58:24 2019]->");
    cliLenTemplate = strlen("[000.000.000.000:00000]->");
    sortedLogsStringSize = loggerToWait*MAX_LOGS_PER_BRANCH*(MAX_LOG_LEN+cliLenTemplate+timeLenTemplate);

    do{

        //printf("Logger manager Waiting for logger to finish\n");

        //a branch must not be removed (with merge operation) during sorting operation
        //sem_wait
        //printf("Waiting on semLoggerManaher_Handler, logger\n");
        if(sem_wait(&semLoggerManaher_Handler) == -1){
            perror("Error in sem_wait (semLoggerManaher_Handler)");
        }

        if(sem_wait(&semOnBranchesNum) == -1){
            perror("Error on sem_wait (semOnBranchesNum)");
            exit(-1);
        }

        loggerToWait = actual_branches_num;

        if(sem_post(&semOnBranchesNum) == -1){
            perror("Error on sem_post (semOnBranchesNum)");
            exit(-1);
        }

        //printf("Working on %d loggers\n", loggerToWait);

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

        //a branch must not be removed (with merge operation) during sorting operation
        //sem_post
        //printf("Posting on semLoggerManaher_Handler, logger\n");
        if(sem_post(&semLoggerManaher_Handler) == -1){
            perror("Error in sem_post (semLoggerManaher_Handler)");
        }

        //writing on disk
        if(writeLogsOnDisk() == -1){
            printf("Error on sort loggers logs\n");
            exit(-1);
        }

        //releasing sortedLogsString
        memset(sortedLogsString, 0, sortedLogsStringSize);

        //releasing loggers
        releaseWaitingLoggers();

        //printf("Loggers released\n");

    }while(1);

}