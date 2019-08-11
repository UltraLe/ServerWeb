//
// Created by ezio on 08/08/19.
//

#include <fcntl.h>
#include <time.h>

int loggerToWait = 0;
//struct log *sortedLogs;
char *sortedLogsString;

int timeLenTemplate;
int cliLenTemplate;



void releaseWaitingLoggers()
{
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

    printf("Called logToSting\n");

    char stringLog[MAX_LOG_LEN+cliLenTemplate+timeLenTemplate];

    //log time in string
    struct tm *localTime;
    localTime = localtime(&(currentLog.log_time));
    sprintf(stringTime, "[%s", asctime(localTime));
    strcpy((stringTime + timeLenTemplate - 3), "]->");

    //client info in string
    sprintf(stringClient, "[%s:%d]->", inet_ntoa(currentLog.client.sin_addr), ntohs(currentLog.client.sin_port));

    //client action in string
    switch(currentLog.log_type){
        case CLIENT_ACCEPTED:
            strcpy(stringMessage, CLIENT_ACCEPTED_S);
            break;
        case CLIENT_DISCONNECTED:
            strcpy(stringMessage, CLIENT_DISCONNECTED_S);
            break;
        case CLIENT_ERROR_READ:
            strcpy(stringMessage, CLIENT_ERROR_READ_S);
            break;
        case CLIENT_REMOVED:
            strcpy(stringMessage, CLIENT_REMOVED_S);
            break;
        case CLIENT_SERVED:
            strcpy(stringMessage, CLIENT_SERVED_S);
            break;
        case NO_ACTIVITY:
            strcpy(stringMessage, NO_ACTIVITY_S);
            break;
        default:
            printf("Something went wrong in logToString\n");
            return -1;
    }

    //creating log string
    sprintf(stringLog, "%s%s%s", stringTime, stringClient, stringMessage);

    printf("stringLog: %s\n", stringLog);

    //linking to the sorted ones
    strcat(sortedLogsString, stringLog);

    return 0;
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
    time_t min = time(0) + 1;
    time_t tempLogClock;

    int j;
    int minIndex;

    struct log *tempLog;
    struct log minLog;

    //if any branch has not registered any log,
    //min log has to specify it
    minLog.log_type = NO_ACTIVITY;
    minLog.log_time = time(0);
    minLog.client.sin_port = htons(SERVER_PORT);
    minLog.client.sin_addr.s_addr = htonl(SERVER_ADDR);

    printf("Sorting\n");

    //TODO to solve stack smasching

    for (int i = 0; i < loggerToWait; ) {

        j = 0;
        minIndex = 0;

        printf("After first for\n");

        for (struct branches_info_list *current = first_branch_info; current != NULL && branchCompleted[j] == 0; current = current->next, ++j) {

            printf("Branch %d\n", (current->info)->branch_pid);

            printf("Index: %d\n", branchIndex[j]);

            tempLog = ((current->info)->loggerLogs);

            //if there are no more logs for the current branch,
            //increase i, and continue
            if( ((tempLog[branchIndex[j]]).log_type) == -1){
                ++i;
                branchCompleted[j] = 1;
                continue;
            }

            tempLogClock = (tempLog[branchIndex[j]]).log_time;

            if (tempLogClock < min) {
                min = tempLogClock;
                minIndex = j;
                minLog = *tempLog;
            }

        }

        //increasing index of branch that has the minimum (in clock) log
        branchIndex[minIndex]++;

        if(branchIndex[minIndex] == MAX_LOGS_PER_BRANCH && branchCompleted[j] == 0) {
            ++i;
            branchCompleted[j] = 1;
        }

        //here the minimum has been selected, converted and appended
        logToString(minLog);

        printf("After calling logToString\n");

    }

    printf("All: %s\n", sortedLogsString);

    printf("Finished sorting\n");

    return 0;
}



void loggerManager()
{
    timeLenTemplate = strlen("[Sat Aug 10 15:58:24 2019]->");
    cliLenTemplate = strlen("[000.000.000.000:00000]->");

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
        if(writeLogsOnDisk() == -1){
            printf("Error on sort loggers logs\n");
            exit(-1);
        }

        //releasing sortedLogsString
        free(sortedLogsString);

        //releasing loggers
        releaseWaitingLoggers();

    }while(1);

}
